CC=g++
CFLAGS=-std=c++17
INCLUDES=-I/home/epsda/ReactorServer/
LDFLAGS=-lpthread -lfmt -lspdlog -fsanitize=address -g

# 主要目标
all: server client

server:server.cc
	$(CC) $(CFLAGS) $(INCLUDES) -o server server.cc $(LDFLAGS)

client:client.cc
	$(CC) $(CFLAGS) $(INCLUDES) -o client client.cc $(LDFLAGS)

.PHONY: clean
clean:
	rm -f server client