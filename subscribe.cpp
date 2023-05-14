#include "subscribe.hpp"

pack_t Subscribe::get_sub_news(const char *buf, size_t size, struct sockaddr_in saddr) {
	// make a new packet with the news for all subscribers
	pack_t pack;
	struct in_addr addr;
	uint16_t port;
	size_t true_size = size + sizeof(addr) + sizeof(port);

	// make the address and the port
	addr = saddr.sin_addr;
	port = htons(saddr.sin_port);

	// create a buffer to put all the data inside
	shared_ptr<char[]>new_buf(new char[true_size], default_delete<char[]>());

	memcpy(new_buf.get(), &addr, sizeof(addr));
	memcpy(new_buf.get() + sizeof(addr), &port, sizeof(port));
	memcpy(new_buf.get() + sizeof(addr) + sizeof(port), buf, size);

	// initialize pack
	pack.data = new_buf;
	pack.size = true_size;

	return pack;
}

void Subscribe::send_pack_to_sub(Multi *multi, pack_t pack) {
	string topic = string(pack.data.get() + sizeof(struct in_addr) + sizeof(uint16_t));
	// go through all subscribers of a map and the send them the packet with the news
	for (auto it : topic_map[topic]) {
		if (!subs[it.first].connected) {
			if (!(it.second))
				continue;
			subs[it.first].subq.push(pack); // if a sf = 1 subscriber is not on store the packets
		} else {
			multi->tcp.sendTCP(subs[it.first].fd, pack.data.get(), pack.size, 0); // send the the data
		}
	}
}

void Subscribe::empty_sub(Multi *multi) { // when a sf = 1 subcriber reconnects
	// send them all the news they missed
	sub s = subs[id_map[multi->getFD()]];
	pack_t pack;
	while (!s.subq.empty()) { // while they still have packets to receive
		pack = s.subq.front();
		s.subq.pop();
		multi->tcp.sendTCP(s.fd, pack.data.get(), pack.size, 0); // send them to it
	}
}

char Subscribe::get_pack_type(int fd, char *buf, size_t size) { // go through the packets and get the type
	// and set it up in the according maps
	client_packet pack;
	// get the pack to have acces to the type
	memcpy(&pack, buf, size);

	*(((char *)&pack) + size) = 0;

	if (pack.type == TYPE_AUTH) {
		client_packet_auth *auth = (client_packet_auth *)(&pack);

		if (subs[auth->id].connected) { // if it s already connected ignore
			subs[auth->id].connected = TYPE_AUTH_DUPLICATE;
			return TYPE_AUTH_DUPLICATE;
		}

		if (!subs.count(auth->id)) { // if it never connected before 
			sub new_sub;
			new_sub.connected = 0;
			new_sub.fd = -1;
			subs[auth->id] = new_sub; // add him to the subscriber map
		}

		subs[auth->id].connected = 1; // for the momment is connected
		subs[auth->id].fd = fd; 
		id_map[fd] = auth->id; // add his id to fd map
	} else if (pack.type == TYPE_SUB || pack.type == TYPE_SUB_SF) {
		if (!id_map.count(fd)) // if he didn't authetificate we can't subscribe
			return TYPE_NOT_AUTH;

		client_packet_sub *topic = (client_packet_sub *)&pack;

		// ADD the SF type to the topic map at its id
		if (pack.type == TYPE_SUB)
			topic_map[topic->topic][id_map[fd]] = 0;
		if (pack.type == TYPE_SUB_SF)
			topic_map[topic->topic][id_map[fd]] = 1;
	} else if (pack.type == TYPE_UNSUB) {
		if (!id_map.count(fd))
			return TYPE_NOT_AUTH;
		client_packet_sub *topic = (client_packet_sub *)&pack;
		// remove the id sf pair from the topic map
		topic_map[topic->topic].erase(id_map[fd]);
	}

	return pack.type;
}

void Subscribe::confirm_auth(int fd, char *buf) {
	client_packet_auth *pack = (client_packet_auth *)buf;
	struct sockaddr_in addr;
    socklen_t len = sizeof(addr);

	getpeername(fd, (sockaddr *)&addr, &len);
	// print the client that connected
	printf("New client %s connected from %s:%hu.\n", pack->id, inet_ntoa(addr.sin_addr), (uint16_t)ntohs(addr.sin_port), pack->type);
}

void Subscribe::decline_auth(int fd, char *buf) {
	client_packet_auth *pack = (client_packet_auth *)buf;
	// printf that the client was already connected
	printf("Client %s already connected.\n", pack->id);
}

void Subscribe::close_sub(int fd) {
	// print client disconnected
	printf("Client %s disconnected.\n", id_map[fd].c_str());

	// update its status in the sub map and remove its fd id asociation
	subs[id_map[fd]].connected = 0;
	id_map.erase(fd);
}

void parse_news(char *buf, size_t size) {
	// a packet the client receive has addres, port, topic, data_type and a value
	// we can directly get the first 2 from the buffer
	string addr = inet_ntoa(*(struct in_addr*)buf);
	buf += sizeof(struct in_addr); 
    size -= sizeof(struct in_addr);

    string port = to_string(*(uint16_t*)(buf));
	buf += sizeof(uint16_t);
    size -= sizeof(uint16_t);

	// topic is similar 
    char topic[TOPIC_LEN + 1];
	memcpy(topic, buf, TOPIC_LEN);
	topic[TOPIC_LEN] = 0; // asure we have an end to the topic
	buf += TOPIC_LEN;
    size -= TOPIC_LEN;

    string data_type;
    string value;
	char type = *buf; // type needs to be update in function of this one digit number
	buf++;
	size--;

	if (type == DATA_TYPE_INT) {
		data_type = "INT";

		// for this we have a sign
		long long sign = (*(char*)buf) ? -1 : 1;
        buf++;

		// and a simple integer value
		long long val = ntohl(*(uint32_t*)(buf));
        val *= sign;
		// we convert the result to a string
		value = to_string(val);
	} else if (type == DATA_TYPE_SHORT_REAL) {
		data_type = "SHORT_REAL";

		// this is almost the same
		double val = ntohs(*(uint16_t*)(buf));
		// but we also have 2 decimals
		val /= 100;

		// we convert the result to a string
		ostringstream out;
		out.precision(2);
		out << fixed << val;
		value = out.str();
	} else if (type == DATA_TYPE_FLOAT) {
		data_type = "FLOAT";

		// for this we have a sign
		double sign = (*(char*)buf) ? -1 : 1;
        buf++;
		// and a simple integer value
		double val = ntohl(*(uint32_t*)(buf));
		buf += 4;

		// but to obtain the precision
		double exp = *(uint8_t*)buf;
		val /= my_pow(10, exp); // we devide to a power of 10
		val *= sign;

		// we convert the result to a string
		ostringstream out;
		out.precision(10);
		out << val;
		value = out.str();

	} else if (type == DATA_TYPE_STRING) {
		data_type = "STRING";
		value = string(buf, size); // we jsut get a string
	}
 
	// print everyhig
	printf("%s:%s -%s - %s - %s\n", addr.c_str(), port.c_str(), topic, data_type.c_str(), value.c_str());
}

double my_pow(double a, uint8_t e) {
    double res = 1;
    for (int i = 0; i < e; i++) {
        res *= a;
    }
    return res;
}