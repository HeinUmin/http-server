#ifndef SOCKET_H
#define SOCKET_H

#include <arpa/inet.h>
#include <openssl/ssl.h>

int close_socket(int socket_fd);
int init_socket(struct sockaddr_in *sockaddr);
int connect_socket(int socket_fd, struct sockaddr_in *sockaddr);
ssize_t recv_message(int socket_fd, char *buf, int len, SSL *ssl);
ssize_t send_message(int socket_fd, const char *buf, int len, SSL *ssl);
int close_ssl(SSL *ssl, int socket_fd);
SSL_CTX *init_ssl(void);
SSL *connect_ssl(int socket_fd, SSL_CTX *ctx);

#endif
