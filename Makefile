CC = gcc
CFLAGS = -Wall

.PHONY: all
all: server client

server: server.c uring_core.h uring_util.h misc_util.h sock_util.h mem_util.h
	$(CC) $(CFLAGS) -o $@ $^

client: client.c uring_core.h uring_util.h misc_util.h sock_util.h mem_util.h
	$(CC) $(CFLAGS) -o $@ $^