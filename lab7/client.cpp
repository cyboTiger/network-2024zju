#include "client.hpp"

void produce(mySocket& skt) {
    // acquire lock
    std::unique_lock<std::mutex> msgLock(mtx);
    msgQueue.push(skt.mrecv());
    response.notify_all();
}

void consume(packet& pkt) {
    std::unique_lock<std::mutex> msgLock(mtx);
    while(msgQueue.empty()) response.wait(msgLock);
    // Get response from queue and output it
    pkt = msgQueue.front();
    msgQueue.pop();
    
    LOG(INFO) << "[Info] New packet!";
    for(auto & [name, val] : pkt.field_map) {
        std::cout << name << ": " << val << std::endl;
    }
}

int main(int argc, char *argv[])
{
    // init glog
    auto glog = GlogWrapper(argv[0]);
    int clientSocket;
    struct sockaddr_in serverAddress, clientAddress;

    // set self addr
    std::string client_addr_str = CLIENT_ADDRESS;
    uint16_t client_port = CLIENT_PORT;
    set_SocketAddr(&clientAddress, client_port, client_addr_str);
    // create socket
    mySocket skt(clientAddress);
    // set server addr
    std::string server_addr_str = SERVER_ADDRESS;
    uint16_t server_port = SERVER_PORT;
    set_SocketAddr(&serverAddress, server_port, server_addr_str);
    

    // connnect to server
    LOG_IF(FATAL, connect(clientSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
        << "[Error] Connection failed";

    // send request
    packet pkt_send;
    ssize_t len = skt.msend(pkt_send);
    
    // receive msg from server to buf (produce)
    produce(skt);

    // get msg from buf
    packet pkt_recv;
    consume(pkt_recv);

    // close socket
    close(clientSocket);

    return 0;
}