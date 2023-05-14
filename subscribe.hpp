#include "multi.hpp"

using namespace std;

#define DATA_TYPE_INT 0
#define DATA_TYPE_SHORT_REAL 1
#define DATA_TYPE_FLOAT 2
#define DATA_TYPE_STRING 3

struct sub {
	int fd;
	char connected;
	queue<pack_t> subq;
};

class Subscribe {
public:
	// organize address, port and topic information in a packet
	pack_t get_sub_news(const char *buf, size_t size, struct sockaddr_in saddr);
	// send such a packet to all subs and if they are not connected store them into
	void send_pack_to_sub(Multi *multi, pack_t pack); // the sub structure from the map

	void empty_sub(Multi *multi); // send all the data a subscriber missed
	// we add the clients to the subscriber maps and topic in case of it
	char get_pack_type(int fd, char *buf, size_t size);
	
	void confirm_auth(int fd, char* buf); // print client connected
	void decline_auth(int fd, char *buf); // print client was connected already
	void close_sub(int fd); // print client disconnected
private:
	map <string, map <string, char>> topic_map; // (map with sf found by id) found by topic
	map <string, sub> subs; // map with subscriber found by id
	map <int, string> id_map;  // map with id found by fd
};

void parse_news(char *buf, size_t size); // get the data from a topic packet and print it
double my_pow(double a, uint8_t e); // me being lazy again