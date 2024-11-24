#ifndef SERVER_H
#define SERVER_H

#include <arpa/inet.h>

typedef struct {
    int https;
    int socket_fd;
    struct sockaddr_in sockaddr;
} SockInfo;

void *server(void *arg);

#endif
