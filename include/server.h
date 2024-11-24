#ifndef SERVER_H
#define SERVER_H

#include <arpa/inet.h>
#include <openssl/ssl.h>

typedef struct {
    int https;
    int socket_fd;
    struct sockaddr_in sockaddr;
    SSL *ssl;
} SockInfo;

void *server(void *arg);

#endif
