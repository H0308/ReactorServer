CC=g++
CFLAGS=-std=c++17
INCLUDES=-I/home/epsda/ReactorServer/
LDFLAGS=-lpthread -lfmt -lspdlog -fsanitize=address -g

client:client.cc
	$(CC) $(CFLAGS) $(INCLUDES) -o client client.cc $(LDFLAGS)

.PHONY: clean
clean:
	rm -f client