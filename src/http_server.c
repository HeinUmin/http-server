#include "http_server.h"
#include "socket.h"
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

struct Client_info {
    int client_sock;
    struct sockaddr_in sockaddr;
};

void *handle_request(void *arg) {
    struct Client_info *client_info = (struct Client_info *)arg;
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_info->sockaddr.sin_addr, ip_str,
              INET_ADDRSTRLEN);
    printf("Handle request from %s:%d\n", ip_str,
           ntohs(client_info->sockaddr.sin_port));
    // TODO: Implement handle_request
    return (void *)EXIT_SUCCESS;
}

void *http_server(void *arg) {
    int server_sock = 0;
    struct Client_info client_info;
    server_sock = init_socket(8080);
    if (server_sock < 0) {
        close_socket(server_sock);
        return (void *)EXIT_FAILURE;
    }
    printf("Init socket %d\n", server_sock);

    while (1) {
        client_info.client_sock = connect_socket(
            server_sock, (struct sockaddr *)&client_info.sockaddr);
        if (client_info.client_sock < 0) {
            if (errno == EWOULDBLOCK) { continue; }
            close_socket(server_sock);
            return (void *)EXIT_FAILURE;
        }
        printf("Connect socket %d\n", client_info.client_sock);

        pthread_t thread = 0;
        if (pthread_create(&thread, NULL, handle_request,
                           (void *)&client_info) != 0) {
            close_socket(client_info.client_sock);
            close_socket(server_sock);
            return (void *)EXIT_FAILURE;
        }
    }
    return (void *)EXIT_SUCCESS;
}
