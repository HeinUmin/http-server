#include "socket.h"
#include "log.h"
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define ERR_BUF_SIZE 64

int close_socket(int socket_fd) {
    char err_buf[ERR_BUF_SIZE];
    int ret = close(socket_fd);
    if (ret) {
        strerror_r(errno, err_buf, ERR_BUF_SIZE);
        error_log(FATAL, NULL, "close", err_buf);
        return ret;
    }
    if (sprintf(err_buf, "Close socket %d", socket_fd) < 0) {
        strerror_r(errno, err_buf, ERR_BUF_SIZE);
        error_log(WARN, NULL, "snprintf", err_buf);
    } else {
        error_log(INFO, NULL, "close_socket", err_buf);
    }
    return 0;
}

int init_socket(struct sockaddr_in *sockaddr) {
    int reuse = 1;
    char err_buf[ERR_BUF_SIZE];

    // Get socket address
    int socket_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        strerror_r(errno, err_buf, ERR_BUF_SIZE);
        error_log(FATAL, NULL, "socket", err_buf);
        return -1;
    }

    if (sprintf(err_buf, "Init socket %d", socket_fd) < 0) {
        strerror_r(errno, err_buf, ERR_BUF_SIZE);
        error_log(WARN, NULL, "snprintf", err_buf);
    } else {
        error_log(INFO, NULL, "init_socket", err_buf);
    }

    // Set socket reusable
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) <
        0) {
        strerror_r(errno, err_buf, ERR_BUF_SIZE);
        error_log(WARN, NULL, "setsockopt", err_buf);
    } else if (sprintf(err_buf, "Set socket %d reusable", socket_fd) < 0) {
        strerror_r(errno, err_buf, ERR_BUF_SIZE);
        error_log(WARN, NULL, "snprintf", err_buf);
    } else {
        error_log(INFO, NULL, "init_socket", err_buf);
    }

    // Bind socket
    if (sockaddr) {
        if (bind(socket_fd, (struct sockaddr *)sockaddr,
                 sizeof(struct sockaddr_in))) {
            strerror_r(errno, err_buf, ERR_BUF_SIZE);
            error_log(FATAL, NULL, "bind", err_buf);
            close_socket(socket_fd);
            return -1;
        }
        if (sprintf(err_buf, "Bind socket %d", socket_fd) < 0) {
            strerror_r(errno, err_buf, ERR_BUF_SIZE);
            error_log(WARN, sockaddr, "snprintf", err_buf);
        } else {
            error_log(INFO, sockaddr, "init_socket", err_buf);
        }
    }

    // Listen socket
    if (listen(socket_fd, 5)) {
        strerror_r(errno, err_buf, ERR_BUF_SIZE);
        error_log(FATAL, sockaddr, "listen", err_buf);
        close_socket(socket_fd);
        return -1;
    }
    if (sprintf(err_buf, "Listen socket %d", socket_fd) < 0) {
        strerror_r(errno, err_buf, ERR_BUF_SIZE);
        error_log(WARN, sockaddr, "snprintf", err_buf);
    } else {
        error_log(INFO, sockaddr, "init_socket", err_buf);
    }
    return socket_fd;
}

int connect_socket(int server_sock, struct sockaddr_in *sockaddr) {
    char err_buf[ERR_BUF_SIZE];
    int client_sock = 0;
    socklen_t len = sizeof(struct sockaddr_in);
    struct timeval timeout = {3, 0};

    // Accept connection
    client_sock = accept(server_sock, (struct sockaddr *)sockaddr, &len);
    if (client_sock < 0) {
        strerror_r(errno, err_buf, ERR_BUF_SIZE);
        error_log(FATAL, sockaddr, "accept", err_buf);
        close_socket(server_sock);
        return -1;
    }
    if (sprintf(err_buf, "Accept socket %d", client_sock) < 0) {
        strerror_r(errno, err_buf, ERR_BUF_SIZE);
        error_log(WARN, sockaddr, "snprintf", err_buf);
    } else {
        error_log(DEBUG, sockaddr, "connect_socket", err_buf);
    }

    // Set socket timeout
    if (setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
                   sizeof(struct timeval)) ||
        setsockopt(client_sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,
                   sizeof(struct timeval))) {
        strerror_r(errno, err_buf, ERR_BUF_SIZE);
        error_log(WARN, sockaddr, "setsockopt", err_buf);
    } else if (sprintf(err_buf, "Set socket %d timeout", client_sock) < 0) {
        strerror_r(errno, err_buf, ERR_BUF_SIZE);
        error_log(WARN, sockaddr, "snprintf", err_buf);
    } else {
        error_log(DEBUG, sockaddr, "connect_socket", err_buf);
    }
    return client_sock;
}
