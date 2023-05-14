#include <bits/stdc++.h>
#include <unistd.h>
#ifndef UDP_INIT
#define UDP_INIT true
#define MAX_UDP 1551

// THIS CLASS IS A MISTAKE
// this is a glorified int variable

class UDP{
public:
	int getFD();
	void setFD(int fd);
	void closeUDP();
private:
	int fd;
};

#endif