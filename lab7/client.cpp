#include "client.hpp"

void sendInput(mySocket &skt);
void produce(mySocket& skt);
void consume(packet &pkt);
void exitHandler(int signal);
void print_pkt_info(packet &pkt);
bool should_exit = false;
std::mutex print_mtx;
std::map<pkt_t, std::string> packet_type_name = {
    std::make_pair(pkt_t::req_ClientList, "request-ClientList"), 
    std::make_pair(pkt_t::req_Exit, "request-Exit"),
    std::make_pair(pkt_t::req_Msg, "request-Msg"),
    std::make_pair(pkt_t::req_ServerName, "request-ServerName"),
    std::make_pair(pkt_t::req_Time, "request-Time"),
    std::make_pair(pkt_t::req_Unconnect, "request-Unconnect"),
    std::make_pair(pkt_t::res, "response")
    };
std::map<std::string, pkt_t> number_packet_type = {
    std::make_pair("4", pkt_t::req_ClientList), 
    std::make_pair("6", pkt_t::req_Exit),
    std::make_pair("5", pkt_t::req_Msg),
    std::make_pair("3", pkt_t::req_ServerName),
    std::make_pair("2", pkt_t::req_Time),
    std::make_pair("1", pkt_t::req_Unconnect),
    std::make_pair("7", pkt_t::res)
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

        if (input == "q" || number_packet_type[input] == pkt_t::req_Exit
            || number_packet_type[input] == pkt_t::req_Unconnect) {
            pkt.set_type(req_Exit);
            skt.msend(pkt);
            exitHandler(SIGINT);
            break;
        }
        pkt.type = number_packet_type[input]; // 设置消息类型
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
    size_t total_width = (max_name_length + max_value_length + 4) > 30 ? 
                        (max_name_length + max_value_length + 4) : 30; // 4 for padding and separators

    // Print the top border of the box
    std::cout << std::string(total_width, '-') << std::endl;

    // Print the packet information
    std::cout << "| Info: New packet with " << pkt.field_num << " fields! " 
              << std::setw(total_width - 29) << "|" << std::endl; // Adjust width for the message
    std::cout << std::string(total_width, '-') << std::endl;

    std::cout << "| Packet type: " << packet_type_name[pkt.type]
              << std::setw(total_width - 13 - packet_type_name[pkt.type].length()) << "|" << std::endl; // Adjust width for the packet type
    std::cout << std::string(total_width, '-') << std::endl;

    for (const auto & [name, val] : pkt.field_map) {
        std::cout << "| " << std::left << std::setw(max_name_length) << name 
                  << " : " << std::left << std::setw(max_value_length) << val 
                  << " |" << std::endl;
    }

    // Print the bottom border of the box
    std::cout << std::string(total_width, '-') << std::endl;
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
