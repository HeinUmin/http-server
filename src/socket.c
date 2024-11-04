#include "socket.h"
#include <stdlib.h>
#include <unistd.h>

struct sockaddr_in sockaddr[1024];

int close_socket(int server_sock, fd_set *rfds) {
    if (close(server_sock)) {
        FD_CLR(server_sock, rfds);
        return EXIT_FAILURE;
    }
    FD_CLR(server_sock, rfds);
    return EXIT_SUCCESS;
}

int init_socket(fd_set *rfds, uint16_t port) {
    int reuse = 1;
    struct timeval timeout = {3, 0};
    int sock = 0;

    sockaddr[0].sin_port = htons(port);
    sockaddr[0].sin_family = AF_INET;
    sockaddr[0].sin_addr.s_addr = htonl(INADDR_ANY);

    // Get socket address
    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) == -1) { return -1; }
    sockaddr[sock] = sockaddr[0];
    FD_ZERO(rfds);
    FD_SET(sock, rfds);

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
    if (bind(sock, (struct sockaddr *)&sockaddr[sock],
             sizeof(sockaddr[sock]))) {
        close_socket(sock, rfds);
        return 1;
    }

    // Listen socket
    if (listen(sock, 5)) { return -1; }
    return sock;
}

int connect_socket(int server_sock, fd_set *rfds) {
    int client_sock = 0;
    socklen_t cli_size = sizeof(sockaddr[0]);
    if ((client_sock = accept(server_sock, (struct sockaddr *)&sockaddr[0],
                              &cli_size)) == -1) {
        return -1;
    }
    sockaddr[client_sock] = sockaddr[0];
    if (client_sock >= FD_SETSIZE) {
        close_socket(client_sock, rfds);
        return -1;
    }
    FD_SET(client_sock, rfds);
    return client_sock;
}
