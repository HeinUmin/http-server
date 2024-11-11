#ifndef SERVER_H
#define SERVER_H

#include <arpa/inet.h>

typedef struct {
    int socket_fd;
    struct sockaddr_in sockaddr;
} SockInfo;

void *http_server(void *arg);
void *https_server(void *arg);

#endif
