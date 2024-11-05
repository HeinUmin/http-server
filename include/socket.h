#ifndef SOCKET_H
#define SOCKET_H
#include <arpa/inet.h>

int close_socket(int socket_no);
int init_socket(uint16_t port);
int connect_socket(int server_sock, struct sockaddr *sockaddr);

#endif
