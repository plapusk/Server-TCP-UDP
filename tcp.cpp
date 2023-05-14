#include "tcp.hpp"

using namespace std;
int TCP::getFD() {
	return fd;
}

void TCP::setFD(int fd) {
	this->fd = fd;
}

void TCP::clearFD(int fd) { // remove fd from map
	sock_map.erase(fd);
}

int TCP::connectTCP(uint32_t s_addr, uint16_t port) {
	// get tcp socket
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		exit(-1);
	
	int flag = 1; // set socket options
	if (setsockopt(sockfd , IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int)) < 0)
		exit(-1);

	return sockfd;
}

void TCP::add_socket(int sockfd) {
	// add fd to the socket map
	sock_map[sockfd] = make_unique<TCP_buffer>(sockfd);
}


void TCP::sendTCP(int sockfd, void *buf, uint32_t len, int flags) {
	auto sock = sock_map.find(sockfd);
	if (sock == sock_map.end()) // if our fd is on the map
		exit(-1);

	char new_buf[MAX_TCP_LEN];
	uint32_t tcp_len = htonl(len);
	size_t rez;

	if (len + sizeof(tcp_len) > MAX_TCP_LEN)
		exit(-1);

	// make a buffer with the lenght of the data and then the actual data
	memcpy(new_buf, &tcp_len, sizeof(tcp_len));
	memcpy(new_buf + sizeof(tcp_len), buf, len);

	// send the buffere we created to the socket
	rez = send(sockfd, new_buf, len + sizeof(tcp_len), flags);
	if (rez < 0)
		exit(-1);
}

int TCP::acceptTCP() {
	struct sockaddr_in s_addr;
	socklen_t len = sizeof(s_addr);

	// get new TCP socket
	int sockfd = accept(fd, (struct sockaddr *)&s_addr, &len);
	if (sockfd < 0)
		exit(-1);
	
	int flag = 1; // set socket options
	if (setsockopt(sockfd , IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int)) < 0)
		exit(-1);

	// add the fd to the socket map
	sock_map[sockfd] = make_unique<TCP_buffer>(sockfd);
	return sockfd;
}

void TCP::closeTCP() {
	// close all fds from the map
	for (auto it = sock_map.begin(); it != sock_map.end();it++)
		close(it->first);
	sock_map.clear();
	if (fd > 0) // close the original tcp fd
		close(fd);
}

void TCP::generate_auth_packet(char *id, char *buf, uint32_t *len) {
	struct client_packet_auth *pack = (struct client_packet_auth *)buf;
	// make a packet with the type and id of the authentification request
	pack->type = TYPE_AUTH;
	strncpy(pack->id, id, CLIENT_ID);
	*len = sizeof(pack->type) + CLIENT_ID;
}

void TCP::generate_topic_packet(char type, char *topic, char *buf, uint32_t *len) {
	client_packet_sub *pack = (client_packet_sub *)buf;
	// make a packet with the type and topic of the subscription request
	strncpy(pack->topic, topic, TOPIC_LEN);
	pack->type = type;
	*len = sizeof(pack->type) + strnlen(pack->topic, TOPIC_LEN);
}

void TCP::make_duplicate_packet(char *buf, size_t *len) {
	struct client_packet *pack = (struct client_packet *)buf;
	// packet with type duplicate to announce client it s not needed
	pack->type = TYPE_AUTH_DUPLICATE;
	*len = sizeof(pack->type);
}

char TCP::get_tcp_type(char *buf, size_t len) {// get a client packet and get only the type from the buffer
	client_packet *pack = (client_packet *)buf;
	
	size_t new_size = sizeof(pack->type);
    if (len < new_size)
        return TYPE_INVALID;

    return pack->type;
}

bool TCP::is_FD_in_map(int fd) {
	return sock_map.count(fd) > 0;
}

bool TCP::packet_ready(int fd) {
	return sock_map[fd]->is_ready();
}

int TCP::get_fd_type(int fd) {
	char buf[MAX_TCP_LEN];
	ssize_t size;

	// receive data from the fd
	size = ::recv(fd, buf, sizeof(buf), 0);

	// if we didn t receive anyrhing or we didn t manage to read the enitre buffer
	// we received
	if (size <= 0 || sock_map[fd]->read_packet(buf, size) != BUFFER_READ) {		
		sock_map[fd]->reset(); // close and remove the fd
		sock_map.erase(fd);
		close(fd);
		return SOCK_CLOSE;
	}

	return SOCK_TCP;
}



size_t TCP::recv(char *buf, int fd, int flags, int *out) {
	auto sock = sock_map.find(fd);
	if (sock == sock_map.end()) {
		*out = UNBUFFERED_SOCKET;
		return -1;
	}

	
	if (!(sock->second)->is_ready()) {
		*out = PACKET_NOT_READY;
		return 0;
	}

	// if we have the socket in the map and it has a packet ready
	// copy it on the buffer
	return (sock->second)->write_pack(buf);
}