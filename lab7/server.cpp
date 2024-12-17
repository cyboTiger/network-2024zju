#include "server.hpp"

void handleClient(int clientSocket) {
    // 向客户端发送问候消息并关闭连接
    std::string msg = "hello from server!";
    send(clientSocket, msg.c_str(), msg.length(), 0);
    LOG(INFO) << "[Info] Sending message to client " << clientSocket;
    // 关闭sockets
    close(clientSocket);
}

int main(int argc, char* argv[]) {
    // init glog
    auto glog = GlogWrapper(argv[0]);

    int serverSocket, clientSocket;
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t clientAddressLength = sizeof(clientAddress);
    const char* greetingMessage = "Hello from server!";

    // create socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        LOG(ERROR) << "[Error] Failed to create socket";
        return -1;
    }

    // set server addr
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(SERVER_PORT);

    // bind socket to server addr
    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        std::cerr << "[Error] Binding failed" << std::endl;
        return -1;
    }

    // start listening
    LOG(INFO) << "[Info] Listening to connection requests...";
    // max connnection num: MAX_CLIENT_QUEUE
    LOG_IF(FATAL, listen(serverSocket, MAX_CLIENT_QUEUE) < 0) << "[Error] Listening failed";
    std::vector<std::thread> threads;
    // accept client requests
    while(true) {
        // accept new request
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressLength);
        
        LOG_IF(ERROR, clientSocket < 0) << "[Error] A client failed to connect.";

        LOG_IF(INFO, clientSocket >= 0) << "[Info] Client " 
        << std::string(inet_ntoa(clientAddress.sin_addr)) << ":" 
        << clientAddress.sin_port << " has connected";
        // create new thread to handle client
        threads.emplace_back(handleClient, clientSocket);
    }
    
    sleep(6);
    LOG(INFO) << "[Info] Closing server...";
    close(serverSocket);

    return 0;
}

