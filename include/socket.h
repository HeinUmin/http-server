#ifndef SOCKET_H
#define SOCKET_H
#include <arpa/inet.h>

int close_socket(int server_sock, fd_set *rfds);
int init_socket(fd_set *rfds, uint16_t port);
int connect_socket(int server_sock, fd_set *rfds);

#endif
