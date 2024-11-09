#include "log.h"
#include "server.h"
#include "sighandler.h"
#include "socket.h"

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERR_BUF_SIZE 64
#define raise_error()                                                          \
    do {                                                                       \
        if (raise(SIGTERM) != 0) {                                             \
            strerror_r(errno, err_buf, ERR_BUF_SIZE);                          \
            error_log(FATAL, &sockinfo.sockaddr, "raise", err_buf);            \
        }                                                                      \
        return (void *)EXIT_FAILURE;                                           \
    } while (0)

void *handle_request(void *arg) {
    char err_buf[ERR_BUF_SIZE];
    struct Sockinfo client_info = *(struct Sockinfo *)arg;
    pthread_detach(pthread_self());
    if (sprintf(err_buf, "Handle request for socket %d",
                client_info.socket_fd) < 0) {
        strerror_r(errno, err_buf, ERR_BUF_SIZE);
        error_log(WARN, &client_info.sockaddr, "snprintf", err_buf);
    } else {
        error_log(DEBUG, &client_info.sockaddr, "handle_request", err_buf);
    }
    // TODO: Implement handle_request
    remove_thread(pthread_self());
    return (void *)EXIT_SUCCESS;
}

void *http_server(void *arg) {
    char err_buf[ERR_BUF_SIZE];
    int server_sock = 0;
    pthread_t thread = 0;
    struct Sockinfo sockinfo = {
        .socket_fd = 0,
        .sockaddr = {.sin_family = AF_INET,
                     .sin_port = htons(*(uint16_t *)arg),
                     .sin_addr.s_addr = htonl(INADDR_ANY)}};
    // TODO: port number should be configurable

    server_sock = init_socket(&sockinfo.sockaddr);
    if (server_sock < 0) { raise_error(); }
    printf("HTTP server started\n");

    while (1) {
        if (exit_flag) {
            close_socket(server_sock);
            break;
        }
        sockinfo.socket_fd = connect_socket(server_sock, &sockinfo.sockaddr);
        if (exit_flag) { break; }
        if (sockinfo.socket_fd < 0) { raise_error(); }

        if (pthread_create(&thread, NULL, handle_request, &sockinfo) != 0) {
            strerror_r(errno, err_buf, ERR_BUF_SIZE);
            error_log(FATAL, &sockinfo.sockaddr, "pthread_create", err_buf);
            close_socket(sockinfo.socket_fd);
            close_socket(server_sock);
            raise_error();
        } else {
            insert_thread(thread);
        }
    }
    return (void *)EXIT_SUCCESS;
}
