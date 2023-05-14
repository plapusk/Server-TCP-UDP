#include "udp.hpp"

int UDP::getFD() {
	return fd;
}

void UDP::setFD(int fd) {
	this->fd = fd; 
}

void UDP::closeUDP() {
	if (fd >= 0) // close the fd if it exists
		close(fd); 
}