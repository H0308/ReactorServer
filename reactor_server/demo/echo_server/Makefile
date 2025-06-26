CC=g++
CFLAGS=-std=c++17 -O3 -DNDEBUG -march=native -flto=auto
# CFLAGS=-std=c++17
# INCLUDES=-I项目目录
# 例如：INCLUDES=-I/home/epsda/ReactorServer/
# LDFLAGS=-lpthread -lfmt -lspdlog -lboost_system -fsanitize=address -g
LDFLAGS=-lpthread -lfmt -lspdlog -lboost_system -flto=auto

# 主要目标
# all: server client

server:server.cc
	$(CC) $(CFLAGS) $(INCLUDES) -o server server.cc $(LDFLAGS)

# client:client.cc
# 	$(CC) $(CFLAGS) $(INCLUDES) -o client client.cc $(LDFLAGS)

.PHONY: clean
clean:
	rm -f server client