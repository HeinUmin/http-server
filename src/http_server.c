#include "http_server.h"
#include "socket.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

void *http_server(void *arg) {
    fd_set rfds;
    fd_set rset;
    int nready = 0;
    int server_sock = 0;
    int max_fd = 0;
    int client_sock = 0;
    server_sock = init_socket(&rfds, 80);
    if (server_sock < 0) {
        close_socket(server_sock, &rfds);
        return (void *)EXIT_FAILURE;
    } else {
        printf("Init socket %d\n", server_sock);
        max_fd = server_sock;
    }

    while (1) {
        printf("HTTP server running\n");
        rset = rfds;
        nready = select(FD_SETSIZE, &rset, NULL, NULL, NULL);

        if (FD_ISSET(server_sock, &rset)) {
            client_sock = connect_socket(server_sock, &rfds);
            if (client_sock < 0) {
                close_socket(server_sock, &rfds);
                return (void *)EXIT_FAILURE;
            }
            printf("Connect socket %d\n", client_sock);
            if (client_sock > max_fd) { max_fd = client_sock; }
            if (--nready == 0) { continue; }
        }

        for (int i = server_sock + 1; i <= max_fd; i++) {
            if (FD_ISSET(i, &rset)) {
                client_sock = i;
                printf("Handle socket %d\n", client_sock);
                // TODO: Implement http server
                if (--nready == 0) { break; }
            }
        }
    }
    return (void *)EXIT_SUCCESS;
}
