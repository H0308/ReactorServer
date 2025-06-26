CC=g++
CFLAGS=-std=c++17 -O3 -DNDEBUG -march=native -flto=auto
# CFLAGS=-std=c++17
INCLUDES=-I/home/epsda/ReactorServer/
# LDFLAGS=-lpthread -lfmt -lspdlog -lboost_system -fsanitize=address -g
LDFLAGS=-lpthread -lfmt -lspdlog -lboost_system -flto=auto

server:server.cc
	$(CC) $(CFLAGS) $(INCLUDES) -o server server.cc $(LDFLAGS)

.PHONY: clean
clean:
	rm -f server