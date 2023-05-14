#include <bits/stdc++.h>
#include <stdio.h>
#include <unistd.h>
#include "multi.hpp"
#include "subscribe.hpp"

#define BUFLEN 2048 

using namespace std;

int main(int argc, char *argv[]) {
	bool isDone = false;
	int out = 0;
	Subscribe sub;
	char buf[BUFLEN], type;
	size_t len;
	if (argc < 2)
		exit(-1);

	int portno = atoi(argv[1]); // get port from arguments
	if (!portno)
		exit(-1);
	Multi multi = Multi(portno); // initialize the socket handler

	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	multi.enableUDP(); // make UDP fd
	multi.enableTCP(); // make TCP fd

	multi.add_fd(STDIN_FILENO); // add stdin fd

	while(!isDone) {
		multi.get_next_fd(); // get the next fd

		if (multi.getFD() < 0) {
			multi.selection(); // wait for a fd to be available
		} else if (multi.getFD() == STDIN_FILENO) {
			fgets(buf, sizeof(buf) - 1, stdin); // read command from stdin
			if (!strncmp(buf, "exit", 4)) // if exit command
				isDone = true; // end server
		} else if (multi.isUDP()) { // is current fd the UDP one
			struct sockaddr_in addr_udp;
			socklen_t addr_udp_len = sizeof(addr_udp);
			memset(buf, 0, BUFLEN);
			// receive UDP packet from socket
			len = recvfrom(multi.getFD(), buf, MAX_UDP, 0, (struct sockaddr*)(&addr_udp), &addr_udp_len); 
			if (len < 0)
				exit(-1);
			// get the UDP packet and organize it into a packet for my client
			// and then send it
			sub.send_pack_to_sub(&multi, sub.get_sub_news(buf, len, addr_udp));
		} else if (multi.isTCP()) {
			// accept a new TCP socket and add it to the map
			multi.acepting();
		} else if (multi.isClosed()) {
			// disconnect client if socket is closed (also remove fd from id map)
			sub.close_sub(multi.getFD());
		} else {
			// else we recv a packet from tcp
			len = multi.tcp.recv(buf, multi.getFD(), 0, &out);
			if (out == PACKET_NOT_READY || len < 0)
				continue; // skip if we don't have a proper packet

			// we add the clients to the subscriber maps and topic in case of it
			type = sub.get_pack_type(multi.getFD(), buf, len);
			if (type == TYPE_AUTH) {
				// confirm that client is connected
				sub.confirm_auth(multi.getFD(), buf);
				// we announce the client he is accepted onto our server
				multi.tcp.sendTCP(multi.getFD(), buf, len, 0);
				sub.empty_sub(&multi);
			} else if (type == TYPE_AUTH_DUPLICATE) {
				// announce the client is already connected
				sub.decline_auth(multi.getFD(), buf);
				// kindly announce the client to kill itself
				multi.tcp.make_duplicate_packet(buf, &len);
				multi.tcp.sendTCP(multi.getFD(), buf, len, 0);
				// close the fd we created for this client
				multi.closeFD();
			}
		}
	}
	// close sockets and free memory
	multi.close();
	return 0;
}