#include "socket.h"
#include <unistd.h>

int close_socket(int socket_no) {
    int ret = close(socket_no);
    return ret;
}

int init_socket(uint16_t port) {
    int reuse = 1;
    struct timeval timeout = {5, 0};
    struct sockaddr_in sockaddr = {.sin_family = AF_INET,
                                   .sin_port = htons(port),
                                   .sin_addr.s_addr = htonl(INADDR_ANY)};

    // Get socket address
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1) { return -1; }

    // Set socket options
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) ==
            -1 ||
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
                   sizeof(struct timeval)) == -1 ||
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,
                   sizeof(struct timeval)) == -1) {
        return -1;
    }

    // Bind socket
    if (bind(sock, (struct sockaddr *)&sockaddr, sizeof(sockaddr))) {
        close_socket(sock);
        return -1;
    }

    // Listen socket
    if (listen(sock, 5)) { return -1; }
    return sock;
}

int connect_socket(int server_sock, struct sockaddr *sockaddr) {
    int client_sock = 0;
    socklen_t len = sizeof(*sockaddr);
    client_sock = accept(server_sock, sockaddr, &len);
    return client_sock;
}
