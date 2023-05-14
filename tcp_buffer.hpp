#include <bits/stdc++.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <queue>
#include <memory>

#ifndef TCP_BUF
#define TCP_BUF

using namespace std;

#define BUFFER_WRITTEN    0 
#define PACKET_NOT_READY  1
#define BUFFER_TOO_SMALL  2
#define UNBUFFERED_SOCKET 4
#define PACKET_TOO_BIG    8
#define BUFFER_READ       16
#define CONNECTION_CLOSED 32

#define SOCK_CLOSE 16
#define SOCK_TCP 32

#define MAX_TCP_LEN 4096

#define MAX_PACKET_SIZE 65536

struct pack_t {
	uint32_t size;
	std::shared_ptr<char[]> data;
}; // packet with size and a pointer for data 

class TCP_buffer {
public:
	TCP_buffer(int sockfd); // basic constructor
	void reset(); // empty the packets 
	bool is_ready(); // do we have any packets ready
	uint32_t write_pack(char *buf); // put the first packet in our queue into the buffer
	int read_packet(char *buf, uint32_t size); // read everyting from the buffer
	int actual_read(char *&buf, uint32_t &len); // actual read of ONE packet
	int check_size(char *&buf, uint32_t &size); // read the size of the packet from the buffer
	bool size_read(); // see if we read the enitre size
private:
	int sockfd; // the fd
	uint8_t read_idx; // index to see how much we read of the entire size  
	uint32_t exp_size; // what remains from the buffer
	uint32_t buf_size; // the size of the buffer

	shared_ptr<char[]> current_buffer; // we use this buffer while reading a packet
	queue<pack_t> packet_queue; // queue of all packets
};

#endif