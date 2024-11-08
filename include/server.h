#ifndef SERVER_H
#define SERVER_H

#include <arpa/inet.h>

struct Sockinfo {
    int socket_fd;
    struct sockaddr_in sockaddr;
};

void *http_server(void *arg);
void *https_server(void *arg);

#endif
