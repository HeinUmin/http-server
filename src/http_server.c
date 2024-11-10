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

void *handle_request(void *arg) {
    struct Sockinfo client_info = *(struct Sockinfo *)arg;
    insert_thread(pthread_self());
    pthread_detach(pthread_self());
    logd(&client_info.sockaddr, "handle_request",
         "Handle request for socket %d", client_info.socket_fd);
    // TODO: Implement handle_request
    remove_thread(pthread_self());
    return (void *)EXIT_SUCCESS;
}

void *http_server(void *arg) {
    int server_sock = 0;
    pthread_t thread = 0;
    struct Sockinfo sockinfo = {
        .socket_fd = 0,
        .sockaddr = {.sin_family = AF_INET,
                     .sin_port = htons(*(uint16_t *)arg),
                     .sin_addr.s_addr = htonl(INADDR_ANY)}};

    insert_thread(pthread_self());
    server_sock = init_socket(&sockinfo.sockaddr);
    if (server_sock < 0) {
        if (raise(SIGTERM) != 0) {
            log_errno(FATAL, &sockinfo.sockaddr, "raise", errno);
        }
        return (void *)EXIT_FAILURE;
    }
    printf("HTTP server started\n");

    while (1) {
        if (exit_flag) {
            close_socket(server_sock);
            break;
        }
        sockinfo.socket_fd = connect_socket(server_sock, &sockinfo.sockaddr);
        if (exit_flag) { break; }
        if (sockinfo.socket_fd < 0) {
            if (raise(SIGTERM) != 0) {
                log_errno(FATAL, &sockinfo.sockaddr, "raise", errno);
            }
            return (void *)EXIT_FAILURE;
        }
        pthread_create(&thread, NULL, handle_request, &sockinfo);
    }
    remove_thread(pthread_self());
    return (void *)EXIT_SUCCESS;
}
