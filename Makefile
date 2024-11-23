CC = gcc
CFLAGS = -Wall

.PHONY: all
all: server

server: server.c uring_core.h uring_util.h
	$(CC) $(CFLAGS) -o $@ $^
