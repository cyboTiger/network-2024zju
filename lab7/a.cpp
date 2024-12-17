#include "utils.hpp"

int main(int argc, char* argv[]) {
    // init glog
    auto glog = GlogWrapper(argv[0]);

    sockaddr_in serveraddr, clientaddr;
    set_SocketAddr(&serveraddr, 3537, "127.0.0.1");
    set_SocketAddr(&clientaddr, CLIENT_PORT, "127.0.0.1");
    mySocket server(serveraddr);
    LOG_IF(FATAL, listen(server.fd, 20) < 0) 
        << "[Error] Listening failed";
    socklen_t clientaddrlen = sizeof(clientaddr);

    while(true)
    {
        int client = accept(server.fd, (struct sockaddr*)&clientaddr, &clientaddrlen);
    
        packet pkt_recv = server.mrecv();
        // format
        std::cout << std::endl;
        for(int i = 0 ; i < 40 ; ++i) {std::cout << "-"; }
        std::cout << std::endl;

        std::cout << "request type: " << pkt_recv.type << std::endl;
        std::cout << "field num: " << pkt_recv.field_num << std::endl;
        for(auto & [name, val] : pkt_recv.field_map) {
            std::cout << name << ": " << val << std::endl;
        }

        for(int i = 0 ; i < 40 ; ++i) {std::cout << "-"; }
        std::cout << std::endl << std::endl;
        //

        packet pkt_send(req_Msg, 2);
        pkt_send.set_field("hello", pkt_recv.field_map["name"]);
        pkt_send.set_field("access", "granted");
        server.msend(pkt_send);
    }


}