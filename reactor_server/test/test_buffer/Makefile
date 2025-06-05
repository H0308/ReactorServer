CC=g++
CFLAGS=-std=c++17
INCLUDES=-I/home/epsda/ReactorServer/
LDFLAGS=-lpthread -lfmt -lspdlog -fsanitize=address -g

test:test.cc
	$(CC) $(CFLAGS) $(INCLUDES) -o test test.cc $(LDFLAGS)

.PHONY: clean
clean:
	rm -f test