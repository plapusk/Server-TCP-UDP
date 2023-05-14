#include "tcp_buffer.hpp"

TCP_buffer::TCP_buffer(int sockfd) {
	this->sockfd = sockfd;
	read_idx = 0;
}

void TCP_buffer::reset() { // delete everything we had for this socket
	if (size_read())
		current_buffer.reset();
	read_idx = 0;

	while(!packet_queue.empty())
		packet_queue.pop();
}

bool TCP_buffer::is_ready() {
	return !packet_queue.empty();
}

uint32_t TCP_buffer::write_pack(char *buf) { // copy the first packet from the q on the buffer
	pack_t pack = packet_queue.front();
	uint32_t size = pack.size;
	memcpy(buf, pack.data.get(), pack.size);
	packet_queue.pop(); // this line is cursed took me 8h to fix it :(
	return size;
}

int TCP_buffer::read_packet(char *buf, uint32_t size) {
	while (size) // as long as we still have data in the buffer
		if (actual_read(buf, size) == PACKET_TOO_BIG) // read from it
			return PACKET_TOO_BIG;
	return BUFFER_READ;
}

int TCP_buffer::actual_read(char *&buf, uint32_t &len) {
	// any packet i send has the size at the beggining
	if (!size_read()) // this checks if we read that already
		return check_size(buf, len); // otherwise read it

	size_t to_copy = len;
	if (to_copy > exp_size) // if our current packet is shorter than the buffer
		to_copy = exp_size; // update the size so we don't mix packets

	// copy what we can into the buffer
	memcpy(current_buffer.get() + buf_size - exp_size, buf, to_copy);
	exp_size -= to_copy;
	len -= to_copy;
	buf += to_copy;

	if (exp_size == 0) { // if we read an entire packet
		pack_t pack; 
		pack.size = buf_size;
		pack.data = current_buffer;
		packet_queue.push(pack); // add it to the q
		read_idx = 0; // reset idx for new packet

		return BUFFER_WRITTEN;
	}

	return PACKET_NOT_READY;
}

int TCP_buffer::check_size(char *&buf, uint32_t &size) {
	size_t to_copy = sizeof(exp_size) - read_idx; // what remains from the size
	if (sizeof(exp_size) - read_idx > size) // if we somehow don't have the entire size in the buffer
		to_copy = size; // i was getting desprete

	// copy the rest of the size into the size
	memcpy((char *)(&exp_size) + read_idx, buf, to_copy);
	read_idx += to_copy;
	size -= to_copy;
	buf += to_copy;

	if (size_read()) { // if we read the size
		buf_size = ntohl(exp_size); // set the buffer size
		exp_size = ntohl(exp_size); // this becomes what we have left of the packet
		if (buf_size > MAX_PACKET_SIZE)
			return PACKET_TOO_BIG;

		current_buffer = unique_ptr<char[]>(new char[buf_size]); // set up a new buffer to cpy inside
	}
	return PACKET_NOT_READY;
}

bool TCP_buffer::size_read() { // check if we read the size at the beggining of a packet
	return read_idx == sizeof(exp_size);
}