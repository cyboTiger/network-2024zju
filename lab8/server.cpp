// 引入一些必要的头文件
#include <iostream>
#include <string>
#include <cstring> // For memset
#include <unistd.h> // For close
#include <sys/socket.h> // For socket functions
#include <netinet/in.h> // For sockaddr_in
#include <arpa/inet.h>
#include <glog/logging.h> // For glog
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#define MAX_CLIENT_QUEUE 20

#define SERVER_PORT 3538

enum pkt_type {request, response};
class GlogWrapper{ // 封装Glog
public:
    GlogWrapper(char* program) {
        google::InitGoogleLogging(program);
        FLAGS_log_dir="glog"; //设置log文件保存路径及前缀
        FLAGS_alsologtostderr = true; //设置日志消息除了日志文件之外是否去标准输出
        FLAGS_colorlogtostderr = true; //设置记录到标准输出的颜色消息（如果终端支持）
        FLAGS_stop_logging_if_full_disk = true;   //设置是否在磁盘已满时避免日志记录到磁盘
        // FLAGS_stderrthreshold=google::WARNING; //指定仅输出特定级别或以上的日志
        google::InstallFailureSignalHandler();
    }
    ~GlogWrapper() { google::ShutdownGoogleLogging(); }
};

struct packet{
    pkt_type type;
    int field_num;
    std::vector<std::pair<std::string, char[]>> field_pairs;
};

void handleClient(int clientSocket) {
    // 向客户端发送问候消息并关闭连接
    std::string msg = "hello from server!";
    send(clientSocket, msg.c_str(), msg.length(), 0);
    LOG(INFO) << "[Info] Sending message to client " << clientSocket;
    // 关闭sockets
    close(clientSocket);
}

void Serialize(packet* pkt) {

}

int main(int argc, char* argv[]) {


    // 初始化 glog
    auto glog = GlogWrapper(argv[0]);

    int serverSocket, clientSocket;
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t clientAddressLength = sizeof(clientAddress);
    const char* greetingMessage = "Hello from server!";

    // 创建socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "[Error] Failed to create socket" << std::endl;
        return -1;
    }

    // 设置服务器地址信息
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(SERVER_PORT);

    // 绑定socket到本地地址
    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        std::cerr << "[Error] Binding failed" << std::endl;
        return -1;
    }

    // 开始监听客户端的连接请求
    LOG(INFO) << "[Info] Listening to connection requests...";
    // 最大连接数为 MAX_CLIENT_QUEUE
    LOG_IF(FATAL, listen(serverSocket, MAX_CLIENT_QUEUE) < 0) << "[Error] Listening failed";
    std::vector<std::thread> threads;
    // 接受客户端连接
    while(true) {
        // 接受新的连接请求
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressLength);
        LOG_IF(ERROR, clientSocket < 0) << "[Error] A client failed to connect.";
        LOG_IF(INFO, clientSocket >= 0) << "[Info] Client " 
        << std::string(inet_ntoa(clientAddress.sin_addr)) << ":" << clientAddress.sin_port
        << " has connected";
        // 创建新线程处理客户端请求
        threads.emplace_back(handleClient, clientSocket);
    }
    
    sleep(6);
    LOG(INFO) << "[Info] Closing server...";
    close(serverSocket);

    return 0;
}

