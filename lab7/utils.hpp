#include <iostream>
#include <iomanip>
#include <map>
#include <string>
#include <cstring> // For memset
#include <csignal> // exit signal handling
#include <unistd.h> // For close
#include <sys/socket.h> // For socket functions
#include <netinet/in.h> // For sockaddr_in
#include <arpa/inet.h>
#include <glog/logging.h> // For glog
#include <thread> // Concurrency
#include <mutex>
#include <condition_variable>
#include <queue>

// 定义服务端地址和端口
#define SERVER_ADDRESS "127.0.0.1"
#define SERVER_PORT 3537
#define CLIENT_ADDRESS "127.0.0.1"
#define CLIENT_PORT 5678

class GlogWrapper
{ // 封装Glog
public:
    GlogWrapper(char *program)
    {
        google::InitGoogleLogging(program);
        FLAGS_log_dir = "glog";         // 设置log文件保存路径及前缀
        FLAGS_alsologtostderr = true;           // 设置日志消息除了日志文件之外是否去标准输出
        FLAGS_colorlogtostderr = true;          // 设置记录到标准输出的颜色消息（如果终端支持）
        FLAGS_stop_logging_if_full_disk = true; // 设置是否在磁盘已满时避免日志记录到磁盘
        // FLAGS_stderrthreshold=google::WARNING; //指定仅输出特定级别或以上的日志
        google::InstallFailureSignalHandler();
    }
    ~GlogWrapper() { google::ShutdownGoogleLogging(); }
};

struct packet;
size_t Serialize(char* buf, packet& pkt);
void Deserialize(char* buf, packet& pkt);

void set_SocketAddr(struct sockaddr_in* sock_addr, uint16_t port, std::string addr_str) {
    memset(sock_addr, 0, sizeof(sock_addr));
    sock_addr->sin_family = AF_INET;
    sock_addr->sin_port = htons(SERVER_PORT);
    sock_addr->sin_addr.s_addr = inet_addr(addr_str.c_str());
}

enum pkt_t {Unknown, req_Unconnect, req_Time, req_ServerName, 
            req_ClientList, req_Msg, req_Exit, res};

struct packet{
    pkt_t type;
    size_t field_num;
    std::map<std::string, std::string> field_map;

    packet() {field_num = 0; type = pkt_t::Unknown;}
    packet(const packet& other) : 
    type(other.type), field_num(other.field_num) {
        for (auto& [name, val] : other.field_map) {
            set_field(name, val);
        }
    }
    packet(pkt_t type) : type(type) {
        if(type != req_Msg && type != res) {field_num = 0;}
    }
    packet(pkt_t type, int field_num) : type(type), field_num(field_num) {}

    void set_type(pkt_t type) { this->type = type; }
    void set_num(size_t num) { field_num = num; } 
    void set_field(std::string field_name, std::string field_value) {
        if (field_map.find(field_name) != field_map.end()) {
            LOG(INFO) << "[Info] already set this field!";
            return;
        }
        field_map[field_name] = field_value;
    }
    std::string get_field(std::string field_name) {
        if (field_map.find(field_name) == field_map.end()) {
            LOG(INFO) << "[Info] field doesn't exist!";
            return "";
        }
        return this->field_map[field_name];
    }
};

struct mySocket
{
    int fd;
    struct sockaddr_in hostAddress;
    socklen_t hostAddressLength;

    // merely ctor
    mySocket() {
        fd = socket(AF_INET, SOCK_STREAM, 0);
        LOG_IF(FATAL, fd < 0) << "[Error] Failed to create socket";
    }
    // ctor and binding
    mySocket(struct sockaddr_in hostAddress) :
    hostAddress(hostAddress), hostAddressLength(hostAddressLength) 
    {
        fd = socket(AF_INET, SOCK_STREAM, 0);
        LOG_IF(FATAL, fd < 0) << "[Error] Failed to create socket";
        LOG_IF(FATAL, bind(fd, (struct sockaddr*)&hostAddress, sizeof(hostAddress)) < 0)
            << "[Error] Binding failed";  
    }
    ~mySocket() {close(fd);}

    void mconnect(struct sockaddr_in hostAddress) {
        LOG_IF(FATAL, connect(fd, (sockaddr*)&hostAddress, sizeof(hostAddress)) < 0) 
            << "[Error] Failed to connect";
        LOG(INFO) << "[Info] Connected to server at " 
        << SERVER_ADDRESS << ":" << SERVER_PORT;
    }
    size_t msend(packet& pkt) {
        char buf[2048];
        size_t len = Serialize(buf, pkt);
        send(fd, buf, len, 0);
        return len;
    }
    packet mrecv() {
        char buf[2048] = {0};
        recv(fd, buf, sizeof(buf)-1, 0);
        packet pkt;
        Deserialize(buf, pkt);
        return pkt;
    }
};

size_t Serialize(char* buf, packet& pkt) {
    size_t len = 0;

    memcpy(buf+len, &pkt.type, sizeof(pkt.type));
    len += sizeof(pkt.type);
    
    memcpy(buf+len, &pkt.field_num, sizeof(pkt.field_num));
    len += sizeof(pkt.field_num);

    for(auto& [name, val] : pkt.field_map) {
        size_t name_len = name.length();
        memcpy(buf+len, &name_len, sizeof(name.length()));
        len += sizeof(name_len);

        size_t val_len = val.length();
        memcpy(buf+len, &val_len, sizeof(val.length()));
        len += sizeof(val_len);

        memcpy(buf+len, name.c_str(), name.length());
        len += name.length();
        memcpy(buf+len, val.c_str(), val.length());
        len += val.length();
    }
    buf[len] = '\0';
    return len;
}

void Deserialize(char* buf, packet& pkt) {
    pkt_t type;
    memcpy(&type, buf, sizeof(type));
    pkt.set_type(type);
    buf += sizeof(pkt.type);

    size_t field_num;
    memcpy(&field_num, buf, sizeof(pkt.field_num));
    pkt.set_num(field_num);
    buf += sizeof(pkt.field_map.size());

    for(int i = 0 ; i < pkt.field_num ; ++i) {
        size_t name_len;
        size_t val_len;
        memcpy(&name_len, buf, sizeof(name_len));
        buf += sizeof(name_len);
        memcpy(&val_len, buf, sizeof(val_len));
        buf += sizeof(val_len);
        
        std::string name, val;
        for(int j = 0 ; j < name_len + val_len ; ++buf, ++j) {
            if(j < name_len) name += buf[0];
            else val += buf[0];
        }
        pkt.set_field(name, val);
    }
}