#include "client.hpp"

void sendInput(mySocket &skt);
void produce(mySocket& skt);
void consume(packet &pkt);
void exitHandler(int signal);
void print_pkt_info(packet &pkt);
void reconnect(mySocket& skt);
bool should_exit = false;
bool should_reconnect = false;
std::mutex waitMtx;
std::condition_variable waitCond;
std::map<pkt_t, std::string> packet_type_name = {
    std::make_pair(pkt_t::req_ClientList, "request-ClientList"), 
    std::make_pair(pkt_t::req_Exit, "request-Exit"),
    std::make_pair(pkt_t::req_Msg, "request-Msg"),
    std::make_pair(pkt_t::req_ServerName, "request-ServerName"),
    std::make_pair(pkt_t::req_Time, "request-Time"),
    std::make_pair(pkt_t::req_Unconnect, "request-Unconnect"),
    std::make_pair(pkt_t::res, "response")
    };
std::map<std::string, pkt_t> command_packet_type = {
    std::make_pair("ls", pkt_t::req_ClientList), 
    std::make_pair("exit", pkt_t::req_Exit),
    std::make_pair("msg", pkt_t::req_Msg),
    std::make_pair("name", pkt_t::req_ServerName),
    std::make_pair("time", pkt_t::req_Time),
    std::make_pair("t", pkt_t::req_Time),
    std::make_pair("unconnect", pkt_t::req_Unconnect),
    std::make_pair("response", pkt_t::res),
    std::make_pair("res", pkt_t::res)
    };


int main(int argc, char *argv[])
{
    // init glog
    glog.set_name(argv[0]);
    // signal handler
    signal(SIGINT,  exitHandler); // Ctrl + C
    signal(SIGQUIT, exitHandler); // Ctrl + '\'
    signal(SIGHUP,  exitHandler); // current user log off
    int clientSocket;
    struct sockaddr_in serverAddress, clientAddress;
    // create socket
    mySocket client;
    // set server addr
    set_SocketAddr(&serverAddress, SERVER_PORT, SERVER_ADDRESS);
    // connnect to server
    client.mconnect(&serverAddress);
    std::cout << "available command: " << std::endl;
    for (auto &[key, value] : command_packet_type) {
        std::cout << "- " << key << ": " << packet_type_name[value] << std::endl;
    }
    // recv threads
    std::vector<std::thread> threads;
    packet pkt_recv;
    threads.push_back(std::thread(produce, std::ref(client)));
    threads.push_back(std::thread(consume, std::ref(pkt_recv)));
    threads.push_back(std::thread(sendInput, std::ref(client)));
    for(auto& t : threads) { t.join(); }
    
    packet pkt(req_Exit);
    client.msend(pkt);
    close(client.fd);
    return 0;
}
void sendInput(mySocket &skt) {
    while (!should_exit) {
        std::string input;
        packet pkt;

        std::cout << "client> ";
        std::getline(std::cin, input);

        if (input == "reconnect") {
            reconnect(skt);
            continue;
        }
        if (input == "u" || command_packet_type[input] == pkt_t::req_Unconnect) {
            pkt.set_type(req_Unconnect);
            skt.msend(pkt);
            skt.munconnect();
            continue;
        }
        if (input == "me") {
            pkt.set_type(req_SelfFd);
            skt.msend(pkt);
            continue;
        }
        if (input == "q" || command_packet_type[input] == pkt_t::req_Exit) {
            pkt.set_type(req_Exit);
            skt.msend(pkt);
            exitHandler(SIGINT);
            break;
        }
        if (input == "msg") {
            std::cout << "Sending message to which client: ";
            int recv_fd;
            std::cin >> recv_fd;
            pkt.set_type(pkt_t::req_Msg); // 设置消息类型
            pkt.set_num(1);
            pkt.set_field("to", std::to_string(recv_fd));
            skt.msend(pkt);
            continue;
        }
        pkt.set_type(command_packet_type[input]); // 设置消息类型
        skt.msend(pkt);
    }

    std::cout << "Exit!" << std::endl;
}
void produce(mySocket &skt) {
    while (!should_exit) {
        packet pkt;
        size_t len;

        // Receive packet
        len = skt.mrecv(pkt);
        if (len == -1) {
            std::cout << "Unconnected!" << std::endl;
            // Wait for reconnection or exit signal
            std::unique_lock<std::mutex> waitLock(waitMtx);
            while (!should_reconnect && !should_exit) {
                waitCond.wait(waitLock);
            }
            continue;
        }
        if (len == 0) {
            continue;
        }
        if (should_exit) return;

        // Acquire lock only when we have a valid packet
        std::unique_lock<std::mutex> msgLock(mtx);
        msgQueue.push(pkt);
        response.notify_all();  
    }
}

// consume 1 msg from msgQueue, switch to wait if empty queue
void consume(packet &pkt) {
    while (!should_exit) {
        std::unique_lock<std::mutex> msgLock(mtx);

        // Wait until there is a message in the queue or should_exit is true
        response.wait(msgLock, [] { return !msgQueue.empty() || should_exit; });

        if (should_exit) return;

        // Get response from queue and output it
        pkt = msgQueue.front();
        msgQueue.pop();

        // Print packet info outside the critical section to reduce lock contention
        msgLock.unlock();
        print_pkt_info(pkt);
    }
}

void print_pkt_info(packet &pkt) {
    // Determine the maximum length of field names and values
    size_t max_name_length = 0;
    size_t max_value_length = 0;
    for (const auto & [name, val] : pkt.field_map) {
        if (name.length() > max_name_length) {
            max_name_length = name.length();
        }
        if (val.length() > max_value_length) {
            max_value_length = val.length();
        }
    }

    // Calculate the total width of the box
    size_t total_width = (max_name_length + max_value_length + 6) > 40 ? 
                        (max_name_length + max_value_length + 6) : 40; // 4 for padding and separators

    // Print the top border of the box
    std::cout << std::string(total_width, '-') << std::endl;

    // Print the packet information
    std::cout << "| Info: New packet with " << pkt.field_num << " fields! " 
              << std::setw(total_width - 34) << "|" << std::endl; // Adjust width for the message
    std::cout << std::string(total_width, '-') << std::endl;

    std::cout << "| Packet type: " << packet_type_name[pkt.type]
              << std::setw(total_width - 15 - packet_type_name[pkt.type].length()) << "|" << std::endl; // Adjust width for the packet type
    std::cout << std::string(total_width, '-') << std::endl;

    for (const auto & [name, val] : pkt.field_map) {
        std::cout << "| " << std::left << std::setw(max_name_length) << name 
                  << " : " << std::left << std::setw(max_value_length) << val 
                  << std::right << std::setw(total_width-max_name_length-max_value_length - 5) << "|" << std::endl;
    }

    // Print the bottom border of the box
    std::cout << std::string(total_width, '-') << std::endl;
}

void reconnect(mySocket &skt) {
    std::cout << "Reconnecting..." << std::endl;
    struct sockaddr_in serverAddress;
    // set server addr
    set_SocketAddr(&serverAddress, SERVER_PORT, SERVER_ADDRESS);
    skt.mconnect(&serverAddress);

    std::lock_guard<std::mutex> lock(waitMtx);
    should_reconnect = true;
    waitCond.notify_all();
}
void exitHandler(int signal) {
    should_exit = true;

    // Notify all waiting threads to wake up and check should_exit
    response.notify_all();

    // Handle remaining messages in the queue
    while (!msgQueue.empty()) {
        packet pkt = msgQueue.front();
        msgQueue.pop();
        print_pkt_info(pkt);
    }
}
