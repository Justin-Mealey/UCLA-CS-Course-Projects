#pragma once

#include <stdint.h>
#include <unistd.h>
#include <chrono>



using std::chrono::time_point;
using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using std::chrono::seconds;

class Timer {
public:
    Timer() : start(high_resolution_clock::now()) {}

    bool retransmit() {
        auto now = high_resolution_clock::now();
        return !stopped && (duration_cast<seconds>(now - start) > seconds(1));
    }

    void restart(){
        start = high_resolution_clock::now();
        stopped = false;
    }

    void stop(){
        stopped = true;
    }
private:
    time_point<high_resolution_clock> start;
    bool stopped = false;
};

// Main function of transport layer; never quits
void listen_loop(int sockfd, struct sockaddr_in* addr, int type,
                 ssize_t (*input_p)(uint8_t*, size_t),
                 void (*output_p)(uint8_t*, size_t));


int receive_packet(int sockfd, sockaddr_in* addr, packet* pkt);

//Return 1 if we need to fast_retransmit, 0 otherwise
int fast_retransmit(uint16_t curAck);