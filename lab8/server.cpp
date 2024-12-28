#include "server.hpp"
void clientHandler(int client_fd);
void test2Handler(int client_fd, std::string& buf);
void test3Handler(int client_fd, HTTPRequest& req);
void test4Handler(int client_fd, HTTPRequest& req);
void getHandler(int client_fd, HTTPRequest& req);
void postHandler(int client_fd, HTTPRequest& req);
bool isConnected(int sockfd);
void clientHandler2(int client_fd);


int main(int argc, char* argv[]) {
    // init glog
    glog.set_name(argv[0]);

    sockaddr_in serveraddr, clientaddr;
    set_SocketAddr(&serveraddr, SERVER_PORT, SERVER_ADDRESS);
    
    mySocket server(serveraddr);

    // Set SO_REUSEADDR option
    int optval = 1;
    setsockopt(server.fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    socklen_t clientaddrlen = sizeof(clientaddr);

    LOG_IF(FATAL, listen(server.fd, MAX_CLIENT_QUEUE) < 0) 
        << "[Error] Listening failed";
    std::cout << "Listening..." << std::endl;
    std::vector<std::thread> client_threads;

    while(1)
    {
        int client = accept(server.fd, (struct sockaddr*)&clientaddr, &clientaddrlen);
        if(client >= 0){
            std::cout << "[Info] Client " 
            << client << ", addr: " << std::string(inet_ntoa(clientaddr.sin_addr)) << ":" 
            << ntohs(clientaddr.sin_port) << " has connected" << std::endl;

            server.client_fd_list.push_back(client);

            std::thread t(clientHandler, client);
            t.detach();
            // client_threads.emplace_back(clientHandler2, client, 4);
        }
    }
    std::cout << "Main finished" << std::endl;
    return 0;
}

void clientHandler2(int client_fd) {
    char buf[4096] = {0};
    // while(1) {
        auto result = recv(client_fd, buf, sizeof(buf)-1, 0);
        
        std::cout << "recv: " << result << std::endl;
        std::string buf_str(buf);
        HTTPRequest req(buf_str);
        test4Handler(client_fd, req);
    // }
    close(client_fd);
}
void clientHandler(int client_fd) {
    char buf[4096] = {0};
    
        recv(client_fd, buf, sizeof(buf)-1, 0);
        std::string buf_str(buf);
        HTTPRequest req(buf_str);
        
        std::string version = "HTTP/1.0";
        int code = 200;
        std::string reasonPhrase = "OK";
        if(req.method == "GET") {
            HTTPResponse res(version, code, reasonPhrase, req);
            
            send(client_fd, res.serialize().c_str(), res.serialize().size(), 0);
        } else if (req.method == "POST") {
            HTTPResponse res(version, code, reasonPhrase, req);
            send(client_fd, res.serialize().c_str(), res.serialize().size(), 0);
        }
    
    close(client_fd);
}


void getHandler(int client_fd, HTTPRequest& req) {
    std::string version = "HTTP/1.1";
    int code;
    std::string reasonPhrase;
    if(url2path.find(req.url) != url2path.end()) {
        code = 200;
        reasonPhrase = "OK";
    } else {
        code = 404;
        reasonPhrase = "Not Found";
    }
    HTTPResponse res(version, code, reasonPhrase, req);
    LOG_IF(FATAL, send(client_fd, res.serialize().c_str(), res.serialize().size(), 0) < 0) 
        << "client " << client_fd << " send error: " << strerror(errno) << std::endl;

}
void postHandler(int client_fd, HTTPRequest& req) {
    std::string version = "HTTP/1.1";
    int code;
    std::string reasonPhrase;
    if(req.url == "/dopost") {
        code = 200;
        reasonPhrase = "OK";
    } else {
        code = 404;
        reasonPhrase = "Not Found";
    }
    HTTPResponse res(version, code, reasonPhrase);
    LOG_IF(FATAL, send(client_fd, res.serialize().c_str(), res.serialize().size(), 0) < 0) 
        << "client " << client_fd << " send error: " << strerror(errno) << std::endl;

}

// request header + request line
void test2Handler(int client_fd, std::string& buf) { 
    size_t pos1 = buf.find_first_of("\r\n");
    size_t pos2 = buf.find_first_of("\r\n\r\n");
    std::string response = "HTTP/1.0 200 OK\r\nConnection: close\r\n\r\n";
    response += buf.substr(pos1+2, pos2-pos1-2);
    response += buf.substr(0, pos1);
    response += "\r\n";
    std::cout << response << std::endl;
    send(client_fd, response.c_str(), response.size(), 0);
}
// web echo
void test3Handler(int client_fd, HTTPRequest& req) { 
    std::string version = "HTTP/1.0";
    int code = 200;
    std::string reasonPhrase = "OK";
    std::string content_type = req.get_content_type();
    size_t content_length = req.get_content_length();
    std::string content = req.get_content_in_string();
    HTTPResponse res(version, code, reasonPhrase,
                     content_type, content_length, content);
    send(client_fd, res.serialize().c_str(), res.serialize().size(), 0);
}
// path echo
void test4Handler(int client_fd, HTTPRequest& req) { 
    std::string version = "HTTP/1.0";
    HTTPResponse res(version, req);
    std::cout << res.serialize() << std::endl;
    auto result = send(client_fd, res.serialize().c_str(), res.serialize().size(), 0);
    std::cout << "send: " << result << std::endl;
}
bool isConnected(int sockfd) {
    int error = 0;
    socklen_t len = sizeof(error);
    if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
        close(sockfd);
    }
    return error == 0;
}