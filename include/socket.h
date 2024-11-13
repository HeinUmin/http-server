#ifndef SOCKET_H
#define SOCKET_H

#include <arpa/inet.h>

int close_socket(int socket_fd);
int init_socket(struct sockaddr_in *sockaddr);
int connect_socket(int socket_fd, struct sockaddr_in *sockaddr);
ssize_t recv_message(int socket_fd, char *buf, size_t len);

#endif
