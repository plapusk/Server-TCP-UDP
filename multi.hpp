#include <queue>
#include "tcp.hpp"
#include "udp.hpp"

#ifndef MULTI_INIT
#define MULTI_INIT

#define MAX_SUBS 10

#define SOCK_UDP 64
#define SOCK_CONNECT 56
#define FD_UNBUFFERED 512

struct hold_recv {
	int fd;
	int type;
};

class Multi{
public:
	Multi(); // default constructor
	Multi(int port); // same constructor but we have a port

	void enableUDP(); // get UDP fd
	void enableTCP(); // get a TCP fd

	bool isUDP(); // check if fd is UDP
	bool isTCP(); // check if fd is TCP
	bool isClosed(); // check if fd is closed

	int getFD(); // get current fd
	void closeFD(); // close the current fd

	int try_connect(uint32_t s_addr, uint16_t port); // connect to the given port
	void selection(); // add all the fds that are ready to the q
	void add_to_q(fd_set *fd_select); // add any selected fd to the q

	int get_next_fd(); // select the first ready fd from the q
	void add_fd(int fd); // add a fd to fd_set (this is only used for STDIN FILENO)

	void acepting(); // accept a new socket
	void close(); // close fds and the fd_set
	// this is usseless but i am indeed sad
	void test(int sad);

	// these are here cause i am lazy
	UDP udp;
	TCP tcp;
private:
	int fd; // the fd that we are currently dealing with
	int fd_type; // type of the fd
	int fdmax; // the biggest fd we had so far
	fd_set fd_read; // this is a list of fds for the select

	struct sockaddr_in serv_addr;
	queue<hold_recv> q; // queue of fds that we have to deal with
};

#endif