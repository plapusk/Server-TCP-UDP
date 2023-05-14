#include <unordered_map>
#include "tcp_buffer.hpp"

#ifndef TCP_INIT
#define TCP_INIT

#define CLIENT_ID 11
#define CLI_PACKET_SIZE 64
#define TOPIC_LEN 50

#define TYPE_AUTH 1
#define TYPE_SUB 2
#define TYPE_SUB_SF 3
#define TYPE_UNSUB 4
#define TYPE_AUTH_DUPLICATE 5
#define TYPE_NOT_AUTH  6
#define TYPE_INVALID 7

using namespace std;

typedef struct client_packet { // this packet it when we only need the type
	char type;
	char data[CLI_PACKET_SIZE];
} client_packet;

typedef struct client_packet_auth { // this is to request autetification on server
	char type;
	char id[CLIENT_ID]; 
	char data[CLI_PACKET_SIZE - CLIENT_ID];
} client_packet_auth;

typedef struct client_packet_sub { // make request regarding subscribing to topics
	char type;
	char topic[TOPIC_LEN]; 
	char data[CLI_PACKET_SIZE - TOPIC_LEN];
} client_packet_sub;

class TCP{
public:
	int getFD(); // getter
	void setFD(int fd); // setter
	void clearFD(int fd); // remove fd from socket map
	
	int connectTCP(uint32_t s_addr, uint16_t port); // get the TCP socket and set it up
	void sendTCP(int sockfd, void *buf, uint32_t len, int flags); // send a buffer to a socket
	int acceptTCP(); // receive a new fd for a TCP connection
	void closeTCP(); // close all the fds from the map and clear it (plus the original TCP fd)

	void add_socket(int sockfd); // add a socket to the map
	
	void generate_auth_packet(char *id, char *buf, uint32_t *len); // generate a buffer for an autetification request
	void generate_topic_packet(char type, char *topic, char *buf, uint32_t *len); // generate a buffer for a subscription request
	void make_duplicate_packet(char *buf, size_t *len); // make a packet with the type duplicate
	char get_tcp_type(char *buf, size_t len); // get one of these formats buffers and extract the type from them

	bool is_FD_in_map(int fd); // is the fd in the socket map
	// this functions receives a buffer from a socket and then setups the 
	// packages in the socket_map
	int get_fd_type(int fd); // the name only categorizes what it returns
	bool packet_ready(int fd); // checks if this fd has any packets ready

	size_t recv(char *buf, int fd, int flags, int *out); // this extracts any packets that are ready from the map
private:
	// map with all the fd and their respective packets
	unordered_map<int, unique_ptr<TCP_buffer>> sock_map;
	int fd; // the tcp fd
};

#endif