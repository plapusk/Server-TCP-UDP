#include "multi.hpp"
#include "subscribe.hpp"

#define BUFLEN 2048 
#define MAX_COMMAND_SIZE 256

using namespace std;

int get_command (char *type, char *topic, char *buf, size_t size) {
    char *p;

	p = strtok(buf, " \n"); // go through the words in the buffer
	if (!strcmp(p, "subscribe")) // get keywords
		*type = TYPE_SUB;    
	else if (!strcmp(p, "unsubscribe"))
		*type = TYPE_UNSUB;    
	else
		return -1;

	p = strtok(NULL, " \n");
	if (p == NULL) { // don t get a topic is invalid
		return -1;
	}
	strncpy(topic, p, TOPIC_LEN);

	if (*type == TYPE_UNSUB) {
		return 0;
	}

	p = strtok(NULL, " \n");
	if (p == NULL) // check for SF
		return -1;
	if (p[0] == '1')
		*type = TYPE_SUB_SF;
	else
		*type = TYPE_SUB;
	

	return 0;
}

int main(int argc, char *argv[]) {
	int fd, ret, out;
	bool isDone = false;

	char buf[BUFLEN];
	uint32_t buf_len;

	Multi multi;

	char id[CLIENT_ID];
	char topic[TOPIC_LEN];
    char type;

	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	if (argc < 4)
		exit(-1);

	if (strlen(argv[1]) >= CLIENT_ID)
		exit(-1);

	// get arguments
	strcpy(id, argv[1]); // get id

	in_addr addr_ip;
	if(!inet_aton(argv[2], &addr_ip)) // get ip
		exit(-1);

	uint16_t port = htons(atoi(argv[3])); // get port

	// setup a file descriptor
	int sockfd_tcp = multi.try_connect(addr_ip.s_addr , port);

	// setup packet to send a request to communicate with the server
	multi.tcp.generate_auth_packet(id, buf, &buf_len);
	multi.tcp.sendTCP(sockfd_tcp, (void *)buf, buf_len, 0);

	multi.selection(); // wait for our fd to get a response
	multi.get_next_fd();

	// receive the response from the server
	ret = multi.tcp.recv(buf, multi.getFD(), 0, &out);
	if (ret < 0)
		exit(-1);

	char ch = multi.tcp.get_tcp_type(buf, ret);
	if (ch == TYPE_AUTH_DUPLICATE) {
		multi.close(); // i am not needed
		return 0; // kms
	} else if (ch != TYPE_AUTH)
		exit(-1); // i don t even know what i am

	multi.add_fd(STDIN_FILENO);

	while(!isDone) {
		
		multi.get_next_fd(); // get next fd

		if (multi.getFD() < 0) {
			multi.selection(); // wait for a fd to be available
			continue;
		}
		if (multi.getFD() == STDIN_FILENO) {
			// read command
			fgets(buf, MAX_COMMAND_SIZE - 1, stdin);

			if (strlen(buf) == 0)
				continue;

			buf[strlen(buf) - 1] = 0;
			if (!strncmp(buf, "exit", 4)) {
				isDone = true; // we did our job
				break;
			}

			// parse the buffer to get a command
			if (get_command(&type, topic, buf, MAX_COMMAND_SIZE) < 0) {
				fprintf(stderr, "Invalid command.\n"); // you are illiterate
				continue;
			}

			// tell the server what topic we want to subscribe too
			multi.tcp.generate_topic_packet(type, topic, buf, &buf_len);
			multi.tcp.sendTCP(sockfd_tcp, (void *)buf, buf_len, 0);

			if (type == TYPE_SUB || type == TYPE_SUB_SF)
				printf("Subscribed to topic.\n"); // confirm we are subed
			else if(type == TYPE_UNSUB)
				printf("Unsubscribed from topic.\n"); // confirm we are unsubeb
		} else if (multi.isClosed()) {
			isDone = true; // server cut us off
			break;
		} else {
			// we received something from the topics we were subscribed to
			ret = multi.tcp.recv(buf, multi.getFD(), 0, &out);
			// we organize and print what we got
			parse_news(buf, ret);
		}
	}
	// close sockets and free memory
	multi.close();
	return 0;
}