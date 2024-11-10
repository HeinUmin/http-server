#ifndef SOCKET_H
#define SOCKET_H

#include <arpa/inet.h>
#include <netinet/in.h>

struct Request_header {
    char name[4096];
    char value[4096];
};

struct Request {
    char http_version[16];
    char http_method[16];
    char http_uri[4096];
    struct Request_header *headers;
    int header_count;
};

int close_socket(int socket_fd);
int init_socket(struct sockaddr_in *sockaddr);
int connect_socket(int server_sock, struct sockaddr_in *sockaddr);
int parse_request(char *buf, struct Request *request);

#endif
