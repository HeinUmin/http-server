#include "log.h"
#include "server.h"
#include "sighandler.h"
#include "socket.h"
#include "utils.h"

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *handle_request(void *arg) {
    ssize_t readret = 0;
    char buf[HTTP_BUF_SIZE];
    Request *request = NULL;
    int socket_fd = ((SockInfo *)arg)->socket_fd;
    sock = ((SockInfo *)arg)->sockaddr;
    free(arg);
    insert_thread(pthread_self());
    pthread_detach(pthread_self());
    logd("handle_request", "Handle request for socket %d", socket_fd);
    memset(buf, 0, HTTP_BUF_SIZE);
    while ((readret = recv(socket_fd, buf, HTTP_BUF_SIZE, 0)) > 0) {
        logd("recv", "Received %ld bytes", readret);
        logt("recv", "%s", buf);
        request = parse_request(buf);
        if (!request) {
            // TODO: Bad request
        }
        // TODO: Implement handle_request
    }

    remove_thread(pthread_self());
    return (void *)EXIT_SUCCESS;
}

void *http_server(void *arg) {
    int server_sock = 0;
    pthread_t thread = 0;
    SockInfo *sockinfo = NULL;
    struct sockaddr_in sockaddr = {.sin_family = AF_INET,
                                   .sin_port = htons(*(uint16_t *)arg),
                                   .sin_addr.s_addr = htonl(INADDR_ANY)};

    insert_thread(pthread_self());
    server_sock = init_socket(&sockaddr);
    if (server_sock < 0) {
        if (raise(SIGTERM)) { log_errno(FATAL, "raise", errno); }
        return (void *)EXIT_FAILURE;
    }
    printf("HTTP server started\n");

    while (1) {
        if (exit_flag) {
            close_socket(server_sock);
            break;
        }
        if (!(sockinfo = malloc(sizeof(SockInfo)))) {
            log_errno(FATAL, "malloc", errno);
            if (raise(SIGTERM)) { log_errno(FATAL, "raise", errno); }
            close_socket(server_sock);
            break;
        }
        sockinfo->socket_fd = connect_socket(server_sock, &sockinfo->sockaddr);
        if (exit_flag) {
            free(sockinfo);
            break;
        }
        if (sockinfo->socket_fd < 0) {
            free(sockinfo);
            if (raise(SIGTERM)) { log_errno(FATAL, "raise", errno); }
            return (void *)EXIT_FAILURE;
        }
        pthread_create(&thread, NULL, handle_request, (void *)sockinfo);
    }
    remove_thread(pthread_self());
    return (void *)EXIT_SUCCESS;
}
