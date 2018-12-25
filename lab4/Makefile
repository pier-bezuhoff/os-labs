CC = gcc
CFLAGS = -Wall
OBJECTS = tcp_server.o tcp_client.o udp_server.o udp_client.o
EXECUTABLES = tcp_server tcp_client udp_server udp_client

$(OBJECTS): sockets.c

tcp_server: tcp_server.o
	$(CC) $(CFLAGS) -o $@ $^

tcp_client: tcp_client.o
	$(CC) $(CFLAGS) -o $@ $^

udp_server: udp_server.o
	$(CC) $(CFLAGS) -o $@ $^

udp_client: udp_client.o
	$(CC) $(CFLAGS) -o $@ $^

.PHONY: all
all: $(EXECUTABLES)
.DEFAULT_GOAL := all

.PHONY: clean
clean:
	-rm $(OBJECTS) $(EXECUTABLES)
