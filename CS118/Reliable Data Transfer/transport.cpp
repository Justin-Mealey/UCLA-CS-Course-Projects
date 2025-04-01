#include <arpa/inet.h>
#include <stdbool.h>
#include <stdint.h>
#include <cerrno>
#include <unistd.h>
#include <array>
#include "consts.h"
#include <list> //For buffers
#include <set>
#include "io.h"
#include <stdlib.h> //for rand
#include "transport.h"
#include <cstring>  //for memcpy
#include <algorithm> //for min

template<typename T>
void print_receive_buf(const T& buf);

template<typename T>
void print_send_buf(const T& buf);

using buf_type = std::array<char, sizeof(packet) + MAX_PAYLOAD>;

int remove_acked(std::list<buf_type>& buf, uint16_t ack, Timer& timer) {//type change to int
    if(buf.empty()) 
        return 0;
    int bytes_acked = 0;
    auto it = buf.begin();
    while(it != buf.end()) {//loop through packets in buffer
        packet* pkt = (packet*)it->data();
        if(ntohs(pkt->seq) < ack) {//packet is useless (already received)
            bytes_acked += ntohs(pkt->length);
            it = buf.erase(it);
        } 
        else{
            break;
        }
    }
    if(buf.empty()){ 
        timer.stop();
    }
    return bytes_acked;
}

//Returns true if the packet was valid, false if no packet was received or it was corrupted
//pkt has data converted to little-endian format
int receive_packet(int sockfd, sockaddr_in* addr, packet* pkt, uint16_t& receiver_window) {
    socklen_t addr_len = sizeof(struct sockaddr_in);
    int bytes_recvd = recvfrom(sockfd, pkt, sizeof(packet) + MAX_PAYLOAD, 
                              0, (struct sockaddr*) addr, &addr_len);
    if(bytes_recvd == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) { 
            //permissible issues
            return 1;
        }
        else{
            close(sockfd);
            return 2; //errno;
        }

    }
    pkt->length = ntohs(pkt->length); //length of payload in bytes

    //Check parity:
    size_t parity = 0;
    for(size_t i = 0; i < pkt->length + sizeof(packet); ++i){
        parity += __builtin_parity(((char*)pkt)[i]); //number of 1-bits modulo 2 of each character
    }
    if(parity % 2 == 1){ //not matching checksum
        return 1; //bad packet
    }

    pkt->seq = ntohs(pkt->seq);
    pkt->ack = ntohs(pkt->ack);
    pkt->win = ntohs(pkt->win);
    pkt->unused = ntohs(pkt->unused);
    receiver_window = pkt->win;
  

    return 0; //valid packet
}

void send_packet(int sockfd, struct sockaddr_in* addr, packet* pkt, bool add_to_buffer, 
                std::list<buf_type>& send_buf) {
    size_t parity = 0;
    for(size_t i = 0; i < ntohs(pkt->length) + sizeof(packet); ++i) {
        parity += __builtin_parity(((char*)pkt)[i]);
    }
    if(parity % 2 == 1) {
        pkt->flags |= (1 << 2);
    }
    if(add_to_buffer) {
        buf_type new_buf;
        std::memcpy(new_buf.data(), pkt, sizeof(packet) + ntohs(pkt->length)); //copy packet into buffer
        send_buf.push_back(new_buf);
    }
    int did_send  = sendto(sockfd, pkt, sizeof(packet) + ntohs(pkt->length), 
                            0, (struct sockaddr*)addr, sizeof(*addr));
    if (did_send < 0){
        close(sockfd); 
        exit(1);
    }
}


int handshake(int sockfd, struct sockaddr_in* addr, int type, buf_type& buf, bool invalid_packet, 
                std::list<buf_type>& send_buf, 
                uint16_t& seq, uint16_t& ack, bool& connection_established){
  
  
    packet* pkt = (packet*)buf.data();
    const uint16_t& flags = pkt->flags;
    bool syn_flag = flags & 1;
    bool ack_flag = (flags >> 1) & 1;

    if (type == CLIENT && send_buf.empty()) { //may need extra case for if need to send data during handshake
        //syn packet
        packet* syn_pkt = (packet*)buf.data();
        std::memset(syn_pkt, 0, sizeof(packet));
        syn_pkt->seq = htons(seq);
        syn_pkt->win = htons(MAX_WINDOW);
        syn_pkt->flags = 1; //SYN bit to 1
        send_packet(sockfd, addr, syn_pkt, true, send_buf);
        seq++; //SYN counts as a byte

        return 0;
    }

    //server response to SYN
    if (type == SERVER && !invalid_packet && syn_flag && !ack_flag) { //SYN set, ACK not set

        //create synack packet
        ack = pkt->seq + 1;
        packet* synack_pkt = (packet*)buf.data();
        std::memset(synack_pkt, 0, sizeof(packet));
        synack_pkt->seq = htons(seq);
        synack_pkt->ack = htons(ack); //client seq add one
        synack_pkt->win = htons(MAX_WINDOW);
        synack_pkt->flags = 3; //SYN + ACK flags 000..11
        send_packet(sockfd, addr, synack_pkt, true, send_buf);
        seq++;

        return 0;
    }

    //client final ACK
    if (type == CLIENT && !invalid_packet && syn_flag && ack_flag) {//both SYN and ACK set
        ack = pkt->seq + 1;
        packet* ack_pkt = (packet*)buf.data();
        std::memset(ack_pkt, 0, sizeof(packet));
        ack_pkt->seq = 0; //no payload -> SEQ = 0
        ack_pkt->ack = htons(ack); 
        ack_pkt->length = 0;
        ack_pkt->win = htons(MAX_PAYLOAD);
        ack_pkt->flags = 2; //ack
        ack_pkt->unused = 0;
        send_packet(sockfd, addr, ack_pkt, false, send_buf);
        send_buf.clear();

        connection_established = true;
        return 0;
    }

    //server receive final ACK
    if (type == SERVER && !invalid_packet && !syn_flag && ack_flag) { //ACK set, SYN not set
        send_buf.clear();
        connection_established = true;
        if(pkt->length){ack += 1;}
        
        return 0;
    }

    return 0;
}

template<typename T>
void process_receive_buffer(T& receive_buf, uint16_t& ack,
                          void (*output_p)(uint8_t*, size_t)) {
    
    while(!receive_buf.empty()) {//receiving buffer has stuff
        const packet* front_pkt = (packet*)(receive_buf.begin()->data());
        if(front_pkt->seq != ack){ //not expecting this one
            break;
        }
        uint8_t temp_buffer[MAX_PAYLOAD]; //create a non-const buffer for output
        std::memcpy(temp_buffer, front_pkt->payload, front_pkt->length); //copy packet into buffer
        output_p(temp_buffer, front_pkt->length);
        ack += 1;
        receive_buf.erase(receive_buf.begin());
    }
}

//Return 1 if we need to fast_retransmit, 0 otherwise
int fast_retransmit(uint16_t curAck){


    static uint16_t lastAck = UINT16_MAX; //dummy value
    static int numDupAcks = 0;

    if (lastAck == UINT16_MAX){ //first time receiving an ACK
        lastAck = curAck;
        return 0;
    }
    else if (lastAck != curAck){ //got a new ACK
        numDupAcks = 0;
        lastAck = curAck;
        return 0;
    }
    else if (lastAck == curAck){ //got a dupACK
        if (numDupAcks == 2){ //this is our 3rd dupACK
            numDupAcks = 0;
            return 1; //tells us to retransmit
        }
        else{
            numDupAcks++;
            return 0;
        }
    }
    return 0;
}

int retransmit(int sockfd, struct sockaddr_in* addr, const std::list<buf_type>& send_buf){
    if(send_buf.empty()){return 0;}
    const auto& buf = *send_buf.begin();
    packet* pkt = (packet*)buf.data();
    int did_send = sendto(sockfd, pkt, sizeof(packet) + ntohs(pkt->length), 
                            0, (struct sockaddr*)addr, sizeof(*addr));
    if (did_send < 0){
        close(sockfd); return errno;
    }
    return 0;
}

//increment above vars or bytes_sent every time we send_to()
void listen_loop(int sockfd, struct sockaddr_in* addr, int type,
                ssize_t (*input_p)(uint8_t*, size_t),
                void (*output_p)(uint8_t*, size_t)) {
    
    //Define comparison function for std::set
    auto cmp = [](const buf_type& left, const buf_type& right) {
        return ((packet*)left.data())->seq < ((packet*)right.data())->seq;
    };

    uint16_t seq = rand() % 1000;
    uint16_t ack = 0;
    uint16_t receiver_window = MAX_PAYLOAD; //Receiver flow window size, initially set to 1024
    uint16_t bytes_sent = 0; //Number of unacked bytes sent
    std::set<buf_type, decltype(cmp)> receive_buf(cmp); //Stores packets that have been received, automatically sorts them using seq

    //Packets that are sent but not acked, done in order so no sorting needed
    //Note that this holds packets as they would be sent (i.e. Big Endian format)
    std::list<buf_type> send_buf;
    
    bool connection_established = false;
    Timer timer{};
    timer.stop();

    buf_type buf = {0}; //std::array makes copying simpler when adding to receive_buf
    packet* pkt = (packet*)buf.data();

    while(!connection_established) {
        //If invalid_packet == 1, then no packet or corrupted packet (2 means error with recv_from)
        //After completion, pkt is little-endian
        //receive_packet also updates receiver_window if the packet wasn't corrupted

        int invalid_packet = receive_packet(sockfd, addr, pkt, receiver_window);
        // if(!invalid_packet && pkt->length){receive_buf.emplace(buf);}
        if(invalid_packet == 2){
            return;
        }
        if(!invalid_packet && pkt->length){output_io(pkt->payload, pkt->length);}
            
        //handshake sets connection_established to true when receving an ACK for a SYN
        //Sets seq and ack to correct values
        //If return value is != 0, then there was an error with send_to
        int error = handshake(sockfd, addr, type, buf, invalid_packet, send_buf,
                             seq, ack, connection_established);
        if(error){return;}
    }

    // uint16_t dup_ack_count = 0;
    // uint16_t last_ack = 0;
    while(true){
        if(timer.retransmit()){
            retransmit(sockfd, addr, send_buf);
        }

        //If invalid_packet == 1, then no packet or corrupted packet (2 means error from recv_from)
        //After completion, pkt is little-endian
        //receive_packet also updates receiver_window if the packet wasn't corrupted
        int invalid_packet = receive_packet(sockfd, addr, pkt, receiver_window);
        if(invalid_packet == 2){
            fprintf(stderr, "Receive error: regular loop. SEQ: %i, ACK: %i \n", seq, ack);
            return;
        } //Error in recv_from

        
        uint16_t r_seq = pkt->seq; 
        uint16_t r_ack = pkt->ack;
        uint16_t length = pkt->length; //length of payload in bytes
        uint16_t r_window = pkt->win;
        uint16_t flags = pkt->flags; //Flags are not Big-endian
        uint8_t* payload = pkt->payload; //It's an array of chars, so endianness doesn't matter
        bool need_ack = false;

        //Process flags:
        bool syn_flag = flags & 1;
        bool ack_flag = (flags >> 1) & 1;
        if (!ack_flag) r_ack = UINT16_MAX; //set r_ack to dummy value if no ACK was meant to be sent
        bool paritybit = (flags >> 2) & 1; //PARITY BIT MUST EQUAL XOR OF ALL BITS IN PACKET

        if(!invalid_packet) {
            need_ack = true;
            if(send_buf.size() && (pkt->flags & 2)) {  // ACK flag set
                int rc = fast_retransmit(r_ack);
                if (rc == 1){
                    retransmit(sockfd, addr, send_buf);
                }                
                bytes_sent -= remove_acked(send_buf, pkt->ack, timer);
            }

            if(pkt->length > 0) {
                need_ack = true;
                if(pkt->seq == ack) {
                    output_p(pkt->payload, pkt->length);
                    ack += 1;
                    process_receive_buffer(receive_buf, ack, output_p);
                    
                } else if(pkt->seq > ack) {
                    receive_buf.insert(buf);
                    
                }
            }
        }
        //FAST RETRANSMIT:
        int before = ack;
        process_receive_buffer(receive_buf, ack, output_p);

        if(before != ack){need_ack = true;}
      
        bool sent = false;
        if(bytes_sent < receiver_window) {
            uint16_t available_window = receiver_window - bytes_sent;
            uint16_t payload_size = std::min<uint16_t>(MAX_PAYLOAD, available_window);
            
            packet* send_pkt = (packet*)buf.data();
            std::memset(send_pkt, 0, sizeof(packet));
            
            ssize_t bytes_read = input_p(send_pkt->payload, payload_size);
            if(bytes_read > 0) {
                send_pkt->seq = htons(seq);
                send_pkt->ack = htons(ack);
                send_pkt->length = htons(bytes_read);
                send_pkt->win = htons(MAX_WINDOW);
                send_pkt->flags = 2;
                send_packet(sockfd, addr, send_pkt, true, send_buf);
                sent = true;
                seq += 1;
                bytes_sent += bytes_read;
            } 
        }
        if(!sent && need_ack){
            packet* ack_pkt = (packet*)buf.data();
            std::memset(ack_pkt, 0, sizeof(packet) + MAX_PAYLOAD);
            ack_pkt->ack = htons(ack);
            ack_pkt->win = htons(MAX_WINDOW);
            ack_pkt->flags = 2;
            send_packet(sockfd, addr, ack_pkt, false, send_buf);
        }
    }
}
