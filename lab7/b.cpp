#include "utils.hpp"
std::mutex mtx;
std::condition_variable response;
std::queue<packet> msgQueue;
void produce(mySocket);
void consume(packet);


int main(int argc, char* argv[]) {
    // init glog
    auto glog = GlogWrapper(argv[0]);

    sockaddr_in serveraddr, clientaddr;
    set_SocketAddr(&serveraddr, 3537, "127.0.0.1");
    set_SocketAddr(&clientaddr, CLIENT_PORT, "127.0.0.1");
    std::cout << "ok";
    mySocket client(clientaddr);

    client.mconnect(serveraddr);
    
    packet pkt_send(req_Msg, 4);
    pkt_send.set_field("name", "harry");
    pkt_send.set_field("age", "20");
    pkt_send.set_field("hobby", "tennis");
    pkt_send.set_field("job", "junior undergrad");
    client.msend(pkt_send);

    packet pkt_recv;
    std::vector<std::thread> threads;
    std::cout << "ok here" << std::endl;
    threads.emplace_back(consume, pkt_recv);
    threads.emplace_back(produce, client);
    
}

int add(int a, int b) {return a+b;};

void produce(mySocket skt) {
    // acquire lock
    std::unique_lock<std::mutex> msgLock(mtx);
    msgQueue.push(skt.mrecv());
    response.notify_all();
}

void consume(packet pkt) {
    std::unique_lock<std::mutex> msgLock(mtx);
    while(msgQueue.empty()) response.wait(msgLock);
    // Get response from queue and output it
    if(!msgQueue.empty()) {
        pkt = msgQueue.front();
        msgQueue.pop();
        LOG(INFO) << "[Info] New packet!";
        for(auto & [name, val] : pkt.field_map) {
            std::cout << name << ": " << val << std::endl;
        }
    } else {
        LOG(INFO) << "message queue empty!";
    }
}