#include "multi.hpp"

Multi::Multi() { // constructor
	fd = -1;
	fd_type = 0;
	fdmax = 0;
	tcp.setFD(-1);
	udp.setFD(-1);

	memset((char *)&serv_addr, 0, sizeof(serv_addr));

	FD_ZERO(&fd_read);
}

Multi::Multi(int port) { // constructor
	fd = -1;
	fd_type = 0;
	fdmax = -1;
	tcp.setFD(-1);
	udp.setFD(-1);

	// but we setup the server address from the port
	memset((char *)&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	FD_ZERO(&fd_read);
}

int Multi::getFD() {
	return fd;
}

void Multi::closeFD() { // close the fd and clear from fd_set
	::close(fd);
	if (FD_ISSET(fd, &fd_read))
		FD_CLR(fd, &fd_read);
	tcp.clearFD(fd); // remove fd from socket map
	fd = -1;
}

bool Multi::isUDP() {
	return fd_type == SOCK_UDP;
}

bool Multi::isTCP() {
	return fd_type == SOCK_CONNECT;
}

bool Multi::isClosed() {
	return fd_type == SOCK_CLOSE;
}

void Multi::enableUDP() {
	// get UDP fd
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	udp.setFD(fd); // set UDP fd
	if (fd < 0)
		exit(-1);

	// bind the fd to our server address
	if (bind(fd, (struct sockaddr*)(&serv_addr), sizeof(struct sockaddr)) < 0)
		exit(-1);

	// set the fd
	FD_SET(fd, &fd_read);
	if (fd > fdmax)
		fdmax = fd;
}

void Multi::enableTCP() {
	// get TCP fd
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	tcp.setFD(fd); // set TCP fd
	if (fd < 0)
		exit(-1);

	// bind fd to the server address
	if (bind(fd, (struct sockaddr*)(&serv_addr), sizeof(struct sockaddr)) < 0)
		exit(-1);

	// lsiten to any conestions if they are ant errors
	if (listen(fd, MAX_SUBS) < 0)
		exit(-1);

	int flag = 1; // set socket options
	if (setsockopt(fd,  IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int)) < 0)
		exit(-1);

	// set the fd
	FD_SET(fd, &fd_read);
	if (fd > fdmax)
		fdmax = fd;
}

int Multi::try_connect(uint32_t s_addr, uint16_t port){
	int fd;
	// get the new tcp fd
	fd = tcp.connectTCP((uint32_t)s_addr, port); 

	// setup the server address
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = port;
	serv_addr.sin_addr.s_addr = s_addr;

	// connect to the server address from our addrress
	if (connect(fd, (struct sockaddr*)(&serv_addr), sizeof(serv_addr)) < 0)
		exit(-1);

	if (fd < 0)
		exit(-1);
	// set the fd and fdmax
	FD_SET(fd, &fd_read);
	if (fd > fdmax)
		fdmax = fd;
	
	// add the socket to the map
	tcp.add_socket(fd);
	return fd;
}

void Multi::selection() {
	fd_set fd_select;
	while (q.empty()) {
		fd_select = fd_read;
		// wait for a fd to receive anyting
		if (select(fdmax + 1, &fd_select, NULL, NULL, NULL) < 0)
			exit(-1);
		// add the fd to the q
		add_to_q(&fd_select);
	}
}

void Multi::add_to_q(fd_set *fd_select) {
	hold_recv add;
	for (int i = 0; i <= fdmax; i++) {
		if (!FD_ISSET(i, fd_select)) // if the fd is set
			continue;
		// get the fd type
		if (tcp.is_FD_in_map(i))
			add.type = tcp.get_fd_type(i); // read packet from fd and see if it is valid
		else if (i == tcp.getFD())
			add.type = SOCK_CONNECT;
		else if (i == udp.getFD())
			add.type = SOCK_UDP;
		else
			add.type = FD_UNBUFFERED;

		if (add.type == SOCK_CLOSE)
			FD_CLR(i, &fd_read); // clear the fd if had to close it

		add.fd = i;
		q.push(add);
	}
}


int Multi::get_next_fd() {
	if (q.empty()) { // we don t have any sockets ready
		fd = -1;
		fd_type = 0;
		return -1;
	}

	hold_recv sock = q.front();

	if (sock.type != SOCK_TCP) { // if our socket is not tcp
		q.pop(); // we don't have to check anythig
		fd = sock.fd;
		fd_type = sock.type;
		return fd;
	}

	while (!tcp.is_FD_in_map(sock.fd) || !tcp.packet_ready(sock.fd)) {
		q.pop(); // for tcp we have to get a socket with a packet ready
		if (q.empty()) {
			fd = -1;
			fd_type = 0;
			return -1;
		}
		sock = q.front();
	}

	fd = sock.fd;
	fd_type = sock.type;
	return fd;
}

void Multi::add_fd(int fd) { // this is only for STDIN_FILENO
    FD_SET(fd, &fd_read); // set the socket
}

void Multi::acepting() {
	int sockfd = tcp.acceptTCP(); // get the new tcp socket
	if (sockfd > fdmax) // update fdmax if we get a bigger socket
		fdmax = sockfd;
	FD_SET(sockfd, &fd_read); // set the socket
}

void Multi::close() { // close all the fds
	tcp.closeTCP();
	udp.closeUDP();
	FD_ZERO(&fd_read);
	fdmax = -1;
}

void Multi::test(int sad) { // this makes me sad
	printf("%d %d\n", q.front().type, q.front().fd);
}