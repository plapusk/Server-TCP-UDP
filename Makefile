CC = g++
CFLAGS = -w -Wall

SERVER_SOURCE_ADDITIONAL_FILES = \
	server.cpp \
	multi.cpp \
	tcp.cpp \
	tcp_buffer.cpp \
	subscribe.cpp \
	udp.cpp \

CLIENT_SOURCE_ADDITIONAL_FILES = \
	client.cpp \
	tcp.cpp \
	tcp_buffer.cpp \
	multi.cpp \
	subscribe.cpp \
	udp.cpp \

all: server subscriber

SERVER_OBJ_ADDITIONAL_FILES = $(SERVER_SOURCE_ADDITIONAL_FILES:.cpp=.o)
CLIENT_OBJ_ADDITIONAL_FILES = $(CLIENT_SOURCE_ADDITIONAL_FILES:.cpp=.o)

# Compile server.cpp
server: $(SERVER_OBJ_ADDITIONAL_FILES)
	$(CC) $(CFLAGS) $^ -o $@

# Compile client.cpp
subscriber: $(CLIENT_OBJ_ADDITIONAL_FILES)
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: clean run_server run_client

%.o : %.cpp %.hpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f server subscriber *.o
