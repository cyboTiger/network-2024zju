#include "server.hpp"
void clientHandler(mySocket &server, int client_fd);
void funcHandler1(packet& pkt_send);
void funcHandler2(packet& pkt_send);
void funcHandler3(packet& pkt_send);
void funcHandler4(packet& pkt_send);
void funcHandler5(packet& pkt_send, int result);
void funcHandler6(packet& pkt_send, mySocket& skt);

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
            client_threads.emplace_back(clientHandler, std::ref(server), client);
        }

    }
}

void clientHandler(mySocket &server, int client_fd) {
    char buf[4096] = {0};
    while(1) {
        recv(client_fd, buf, sizeof(buf)-1, 0);
        std::string buf_str(buf);
        size_t pos1 = buf_str.find_first_of("\r\n");
        size_t pos2 = buf_str.find_first_of("\r\n\r\n");
        std::string req_line = buf_str.substr(0, pos1);
        std::string req_header = buf_str.substr(pos1+2, pos2-pos1-2);
        std::string res = "HTTP/1.0 200 OK\r\nConnection: close\r\n\r\n" ;
        res += req_header;
        res += req_line;
        send(client_fd, res.c_str(), res.length(), 0);
        
    } 
    close(client_fd); 
}
void funcHandler1(packet& pkt_send) {
    // Get current server time
    std::time_t now = std::time(nullptr);
    char buffer[80] = {0};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    pkt_send.set_type(pkt_t::res);
    pkt_send.set_num(1);
    pkt_send.set_field("time", buffer);
}
void funcHandler2(packet& pkt_send) {
    pkt_send.set_type(pkt_t::res);
    pkt_send.set_num(1);
    pkt_send.set_field("server_name", "server1");
}
void funcHandler3(packet& pkt_send) {
    pkt_send.set_type(pkt_t::res);
    pkt_send.set_num(1);
    pkt_send.set_field("exit", "success");
}
void funcHandler4(packet& pkt_send) {
    pkt_send.set_type(pkt_t::res);
    pkt_send.set_num(1);
    pkt_send.set_field("unconnnect", "success");
}
void funcHandler5(packet& pkt_send, int result) {
    pkt_send.set_type(pkt_t::res);
    pkt_send.set_num(1);
    pkt_send.set_field("result", result ? "success" : "fail");
}
void funcHandler6(packet& pkt_send, mySocket &skt) {
    pkt_send.set_type(pkt_t::res);
    pkt_send.set_num(skt.client_fd_list.size());
    for (auto client_fd : skt.client_fd_list) {
        pkt_send.set_field(std::string("client ")+std::to_string(client_fd), std::to_string(client_fd));
    }
}

