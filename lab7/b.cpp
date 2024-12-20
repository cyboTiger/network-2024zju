#include "utils.hpp"
std::mutex mtx;
std::condition_variable response;
std::queue<packet> msgQueue;
void produce(mySocket skt);
void consume(packet pkt);

int main(int argc, char* argv[]) {
    // init glog
    glog.set_name(argv[0]);

    sockaddr_in serveraddr, clientaddr;
    set_SocketAddr(&serveraddr, SERVER_PORT, "127.0.0.1");

    mySocket client;

    client.mconnect(&serveraddr);
    std::vector<std::thread> threads;
    
    packet pkt_recv;        
    
    //while (true) {
        // ------ create and send packet
        packet pkt_send(req_Msg, 4);
        pkt_send.set_field("name", "harry");
        pkt_send.set_field("age", "20");
        pkt_send.set_field("hobby", "tennis");
        pkt_send.set_field("job", "junior undergrad");
        client.msend(pkt_send);
        // ------

        threads.emplace_back(produce, client);
        threads.emplace_back(consume, pkt_recv);
        for(auto & t : threads) {
            t.join();
        }
        packet pkt_send2(req_Exit);
        client.msend(pkt_send2);

    //}

    std::cout << "[Info] main program last statement" << std::endl;
    exit(0);
}

// produce 1 msg into msgQueue, switch to wait if receive failed
void produce(mySocket skt) {
    // acquire lock
    std::unique_lock<std::mutex> msgLock(mtx);
    std::cout << "producer acquires lock" << std::endl;
    packet pkt;
    while(skt.mrecv(pkt) < 0) {
        std::cout << "[Info] producer release mtx due to empty socket" << std::endl;
        response.wait(msgLock);
    }
    // push packet into msgQueue
    msgQueue.push(pkt);
    response.notify_all();
    std::cout << "producer over" << std::endl ;
}

// consume 1 msg from msgQueue, switch to wait if empty queue
void consume(packet pkt) {
    std::unique_lock<std::mutex> msgLock(mtx);
    std::cout << "consumer acquires lock" << std::endl;
    while(msgQueue.empty()) {
        std::cout << "[Info] consumer release mtx due to empty MsgQueue" << std::endl; 
        response.wait(msgLock);
    }

    // Get response from queue and output it
    pkt = msgQueue.front();
    msgQueue.pop();
    std::cout << "[Info] New packet with " << pkt.field_num << " fields!" << std::endl;
    for(auto & [name, val] : pkt.field_map) {
        std::cout << name << ": " << val << std::endl;
    }
    std::cout << "consumer over" << std::endl ;
}
