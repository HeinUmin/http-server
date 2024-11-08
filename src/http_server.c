#include "log.h"
#include "server.h"
#include "socket.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERR_BUF_SIZE 64

void *handle_request(void *arg) {
    char err_buf[ERR_BUF_SIZE];
    struct Sockinfo *client_info = arg;
    if (sprintf(err_buf, "Handle request for socket %d",
                client_info->socket_fd) < 0) {
        strerror_r(errno, err_buf, ERR_BUF_SIZE);
        error_log(WARN, &client_info->sockaddr, "snprintf", err_buf);
    } else {
        error_log(DEBUG, &client_info->sockaddr, "handle_request", err_buf);
    }

    // TODO: Implement handle_request
    return (void *)EXIT_SUCCESS;
}

void *http_server(void *arg) {
    char err_buf[ERR_BUF_SIZE];
    int server_sock = 0;
    pthread_t thread = 0;
    struct Sockinfo client_info = {
        .socket_fd = 0,
        .sockaddr = {.sin_family = AF_INET,
                     .sin_port = htons(8080),
                     .sin_addr.s_addr = htonl(INADDR_ANY)}};
    // TODO: port number should be configurable

    server_sock = init_socket(NULL);
    if (server_sock < 0) { return (void *)EXIT_FAILURE; }
    printf("HTTP server started\n");

    while (1) {
        client_info.socket_fd =
            connect_socket(server_sock, &client_info.sockaddr);
        if (client_info.socket_fd < 0) { return (void *)EXIT_FAILURE; }

        if (pthread_create(&thread, NULL, handle_request, &client_info) != 0) {
            strerror_r(errno, err_buf, ERR_BUF_SIZE);
            error_log(FATAL, &client_info.sockaddr, "pthread_create", err_buf);
            close_socket(client_info.socket_fd);
            close_socket(server_sock);
            return (void *)EXIT_FAILURE;
        }
    }
    return (void *)EXIT_SUCCESS;
}
