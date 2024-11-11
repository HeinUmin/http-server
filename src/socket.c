#include "socket.h"
#include "log.h"
#include <errno.h>
#include <netinet/in.h>
#include <unistd.h>

int close_socket(int socket_fd) {
    int ret = close(socket_fd);
    if (ret) {
        log_errno(FATAL, "close", errno);
        return ret;
    }
    logi("close_socket", "Close socket %d", socket_fd);
    return 0;
}

int init_socket(struct sockaddr_in *sockaddr) {
    int reuse = 1;

    // Get socket address
    int socket_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        log_errno(FATAL, "socket", errno);
        return -1;
    }
    logi("init_socket", "Init socket %d", socket_fd);

    // Set socket reusable
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) <
        0) {
        log_errno(WARN, "setsockopt", errno);
    } else {
        logi("init_socket", "Set reusable to socket %d", socket_fd);
    }

    // Bind socket
    if (sockaddr) {
        if (bind(socket_fd, (struct sockaddr *)sockaddr,
                 sizeof(struct sockaddr_in))) {
            log_errno(FATAL, "bind", errno);
            close_socket(socket_fd);
            return -1;
        }
        sock = *sockaddr;
        logi("init_socket", "Bind socket %d", socket_fd);
    }

    // Listen socket
    if (listen(socket_fd, 5)) {
        log_errno(FATAL, "listen", errno);
        close_socket(socket_fd);
        return -1;
    }
    logi("init_socket", "Listen socket %d", socket_fd);
    return socket_fd;
}

int connect_socket(int server_sock, struct sockaddr_in *sockaddr) {
    int client_sock = 0;
    socklen_t len = sizeof(struct sockaddr_in);
    struct timeval timeout = {3, 0};

    // Accept connection
    client_sock = accept(server_sock, (struct sockaddr *)sockaddr, &len);
    if (client_sock < 0) {
        log_errno(FATAL, "accept", errno);
        close_socket(server_sock);
        return -1;
    }
    logd("connect_socket", "Accept socket %d", client_sock);

    // Set socket timeout
    if (setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
                   sizeof(struct timeval)) ||
        setsockopt(client_sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,
                   sizeof(struct timeval))) {
        log_errno(WARN, "setsockopt", errno);
    } else {
        logd("connect_socket", "Set timeout to socket %d", client_sock);
    }
    return client_sock;
}
