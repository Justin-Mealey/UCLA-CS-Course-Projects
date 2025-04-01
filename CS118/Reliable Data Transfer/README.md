# Description of Work (10 points)

This is new for Project 1. We ask you to include a file called `README.md` that contains a quick description of:

1. the design choices you made,
2. any problems you encountered while creating your solution, and
3. your solutions to those problems.

This is not meant to be comprehensive; less than 300 words will suffice.

# Design Choices:
We made the code modular for readability, splitting up the work and easier debugging. Our main functions are remove_acked 
(clears send buffer of acked packets) receive_packet and send_packet (to process network interaction), handshake (commence 
3-way handshake), process_receive_buffer (write to stdout and remove once inorder), fast_retransmit (tells us whether or not 
to retransmit), retransmit and more. This modularity along with helpful comments made this project more digestable. For the 
parity bit setting/corruption detection, we used the __builtin_parity instead of a bunch of self implemented xors. We chose to 
use std::list for the sending buffer and std::set for the receiving buffer. We chose List for the sending buffer as packets 
need to be stored in transmission order, and Set for the receiving buffer so we could add a custom comparison function to sort 
packets by sequence number. This made it much easier to process out of order packets, as receiving packets were always ordered 
in the receiving buffer. We also used a custom timer to handle retransmissions, and fast retransmit was implemented using 
static variables so we could save the number of duplicate ACKs through several calls.

# Problems and Solutions:
We decided to use cpp, because of its many functionalities: templates, lists, arrays, iterators, etc. Because we weren’t well 
versed on makefiles, it took a while to debug and ensure that our code could at least compile. We figured out that we had to 
use g++ instead of gcc. After learning this, we fixed the Makefile and could proceed with developing/testing our project.
Another issue was trying to set the bits correctly. We accidentally swapped the ack and syn bits when checking, leading to 
misinterpreted packets. Originally, our listen loop was a complicated, jumbled mess. This made it very hard to understand and 
debug. To solve this problem, we broke it into many small functions that we could individually edit. Modularity made debugging 
easier, and our code more understandable. Another problen we had was only sending ACKs when we received a payload. This slowed 
down retransmission if both sides stopped sending data while waiting for out-of-order packets. We were timing out on test 
cases, as not enough ACKs were being sent to trigger a fast retransmit. By changing our code so that we would send an ACK on 
any packet, our performance improved dramatically. Our last problem was confusion with variable, as in transport.cpp we could 
have many ACK variable for the ACK we just received, ACK we need to send, etc. This caused bugs where we used the incorrect 
variable when processing or sending packets. To fix this, we had to improve our variable naming, for example using r_ack to 
denote the ACK we just received. 
