#ifndef _SOCK_UTIL_H
#define _SOCK_UTIL_H

#include <arpa/inet.h>
#include <stdio.h>

int _socket()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd >= 0)
        return fd;

    perror("socket");
    return -1;
}

struct sockaddr_in loopback_address(int port)
{
    struct sockaddr_in address;

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    return address;
}

int server(struct sockaddr_in address)
{
    int server_fd = _socket();
    if (server_fd < 0)
        return -1;

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind");
        return -1;
    }

    if (listen(server_fd, 5) < 0)
    {
        perror("listen");
        return -1;
    }

    return server_fd;
}

int client(struct sockaddr_in address)
{
    int client_fd = _socket();

    if (client_fd < 0)
        return -1;

    if (connect(client_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("connect");
        return -1;
    }

    return client_fd;
}

#endif