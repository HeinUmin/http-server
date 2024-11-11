#ifndef SOCKET_H
#define SOCKET_H

#include <arpa/inet.h>
#include <netinet/in.h>

typedef struct {
    char name[4096];
    char value[4096];
} RequestHeader;

typedef struct {
    char http_version[16];
    char http_method[16];
    char http_uri[4096];
    RequestHeader *headers;
    int header_count;
} Request;

int close_socket(int socket_fd);
int init_socket(struct sockaddr_in *sockaddr);
int connect_socket(int server_sock, struct sockaddr_in *sockaddr);
int parse_request(char *buf, Request *request);

#endif
