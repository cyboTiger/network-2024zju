#include "utils.hpp"
#include <ctime>
void clientHandler(mySocket &server, int client_fd);
void funcHandler1(packet& pkt_send);
void funcHandler2(packet& pkt_send);
void funcHandler3(packet& pkt_send);
void funcHandler4(packet& pkt_send);
void funcHandler5(packet& pkt_send);
void funcHandler6(packet& pkt_send);

int main(int argc, char* argv[]) {
    // init glog
    glog.set_name(argv[0]);

    sockaddr_in serveraddr, clientaddr;
    set_SocketAddr(&serveraddr, SERVER_PORT, "127.0.0.1");
    
    mySocket server(serveraddr);

    // Set SO_REUSEADDR option
    int optval = 1;
    setsockopt(server.fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    socklen_t clientaddrlen = sizeof(clientaddr);

    LOG_IF(FATAL, listen(server.fd, 20) < 0) 
        << "[Error] Listening failed";
    std::cout << "Listening..." << std::endl;
    std::vector<std::thread> client_threads;
    while(true)
    {
        int client = accept(server.fd, (struct sockaddr*)&clientaddr, &clientaddrlen);
        if(client >= 0){
            std::cout << "[Info] Client " 
            << std::string(inet_ntoa(clientaddr.sin_addr)) << ":" 
            << ntohs(clientaddr.sin_port) << " has connected" << std::endl;
            server.client_fd_list.push_back(client);
            client_threads.emplace_back(clientHandler, std::ref(server), client);
        }

    }
}

void clientHandler(mySocket &server, int client_fd) {
    while(1) {
        packet pkt_recv;
        server.mrecv(pkt_recv, client_fd);
        packet pkt_send;
        switch (pkt_recv.type)
        {
        case pkt_t::req_Time:
            funcHandler1(pkt_send);
            server.msend(pkt_send, client_fd);
            break;

        case pkt_t::req_ServerName:
            funcHandler2(pkt_send);
            server.msend(pkt_send, client_fd);
            break;    

        case pkt_t::req_Exit:
            funcHandler3(pkt_send);
            server.msend(pkt_send, client_fd);
            close(client_fd);
            return;
        case pkt_t::req_Unconnect:
            funcHandler4(pkt_send);
            server.msend(pkt_send, client_fd);
            close(client_fd);
            return;
        case pkt_t::req_Msg:
            funcHandler5(pkt_send);
            server.msend(pkt_send, client_fd);
            break;
        case pkt_t::req_ClientList:
            funcHandler6(pkt_send);
            server.msend(pkt_send, client_fd);
            break;
        
        case pkt_t::res:
            break;
        default:
            std::cout << "Unknown packet type" << std::endl;
            break;
        }
    }
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
void funcHandler5(packet& pkt_send) {
    pkt_send.set_type(pkt_t::res);
    pkt_send.set_num(1);
    pkt_send.set_field("message", "received");
}
void funcHandler6(packet& pkt_send) {
    pkt_send.set_type(pkt_t::res);
    pkt_send.set_num(1);
    pkt_send.set_field("message", "received");
}