#ifndef SOCKET_H
#define SOCKET_H

#include <arpa/inet.h>
#include <netinet/in.h>

int close_socket(int socket_fd);
int init_socket(struct sockaddr_in *sockaddr);
int connect_socket(int server_sock, struct sockaddr_in *sockaddr);

#endif
