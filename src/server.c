#include "server.h"
#include "log.h"
#include "parser.h"
#include "sighandler.h"
#include "socket.h"
#include "utils.h"

#include <errno.h>
#include <openssl/types.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int
send_error(int socket_fd, Request *request, Response response, SSL *ssl) {
    int err = 0;
    char buf[HTTP_BUF_SIZE];
    char time_str[TIMESTRLEN];
    if (RESPONSE_CODE[response] < 300) { return 1; }
    get_time(time_str, "%a, %d %b %Y %H:%M:%S GMT", 1);
    err |= snprintf(buf, HTTP_BUF_SIZE, "HTTP/1.%d %d %s\r\n",
                    request->http_version, RESPONSE_CODE[response],
                    RESPONSE_STRING[response]) < 0;
    if (response == MOVED) {
        err |= snprintf(buf + strlen(buf), HTTP_BUF_SIZE - strlen(buf),
                        "Location: https://%s:%hu%s\r\n", request->http_host,
                        port[1], request->http_uri) < 0;
    }
    err |= snprintf(buf + strlen(buf), HTTP_BUF_SIZE - strlen(buf),
                    "Content-Length: 0\r\n") < 0;
    err |= snprintf(buf + strlen(buf), HTTP_BUF_SIZE - strlen(buf),
                    "Date: %s\r\n\r\n", time_str) < 0;
    if (err) {
        log_errno(ERROR, "snprintf", errno);
        close_socket(socket_fd);
        return -1;
    }
    if (send_message(socket_fd, buf, (int)strlen(buf), ssl) < 0) { return -1; }
    return 0;
}

static int
send_file(int socket_fd, Request *request, FileInfo *fileinfo, SSL *ssl) {
    FILE *fp = NULL;
    int status = 0;
    size_t sent = 0, readret = 0;
    ssize_t sendret = 0;
    char buf[HTTP_BUF_SIZE];
    char time_str[TIMESTRLEN];

    memset(buf, 0, HTTP_BUF_SIZE);
    get_time(time_str, "%a, %d %b %Y %H:%M:%S GMT", 1);
    status |= snprintf(buf, HTTP_BUF_SIZE, "HTTP/1.%d 200 OK\r\n",
                       request->http_version) < 0;
    status |= snprintf(buf + strlen(buf), HTTP_BUF_SIZE - strlen(buf),
                       "Connection: %s\r\n",
                       request->connection ? "keep-alive" : "close") < 0;
    status |= snprintf(buf + strlen(buf), HTTP_BUF_SIZE - strlen(buf),
                       "Content-Type: %s\r\n", fileinfo->type) < 0;
    status |= snprintf(buf + strlen(buf), HTTP_BUF_SIZE - strlen(buf),
                       "Content-Length: %zu\r\n", fileinfo->size) < 0;
    status |= snprintf(buf + strlen(buf), HTTP_BUF_SIZE - strlen(buf),
                       "Date: %s\r\n\r\n", time_str) < 0;
    if (status) {
        log_errno(ERROR, "snprintf", errno);
        return 1;
    }

    if (strlen(buf) >= HTTP_BUF_SIZE - 1) {
        loge("send_file", "Buffer overflow");
        return 1;
    }
    sendret = send_message(socket_fd, buf, (int)strlen(buf), ssl);
    if (sendret <= 0) { return -1; }
    if (request->http_method == HEAD) { return 0; }

    fp = fopen(fileinfo->path, "rb");
    if (!fp) {
        log_errno(ERROR, "fopen", errno);
        close_socket(socket_fd);
        return -1;
    }

    while (!feof(fp)) {
        readret = fread(buf, sizeof(char), HTTP_BUF_SIZE, fp);
        if (ferror(fp)) {
            log_errno(ERROR, "fread", errno);
            close_socket(socket_fd);
            status = -1;
            break;
        }
        sendret = send_message(socket_fd, buf, (int)readret, ssl);
        if (sendret < 0) {
            status = -1;
            break;
        }
        sent += sendret;
    }

    if (!status && sent != fileinfo->size) {
        loge("send_file", "Sent %zu bytes, expected %zu bytes", sent,
             fileinfo->size);
        close_socket(socket_fd);
        status = -1;
    }
    if (fclose(fp)) { log_errno(ERROR, "fclose", errno); }
    return status;
}

void *handle_request(void *arg) {
    Request request = {0};
    Response response = OK;
    FileInfo fileinfo = {0};
    char buf[HTTP_BUF_SIZE];
    int ret = 0;
    int status = EXIT_SUCCESS;
    int https = ((SockInfo *)arg)->https;
    int socket_fd = ((SockInfo *)arg)->socket_fd;
    SSL *ssl = https ? ((SockInfo *)arg)->ssl : NULL;
    sock = ((SockInfo *)arg)->sockaddr;
    free(arg);
    insert_thread(pthread_self());
    pthread_detach(pthread_self());
    logd("handle_request", "Handle request for socket %d", socket_fd);
    while (recv_message(socket_fd, buf, HTTP_BUF_SIZE, ssl) > 0) {
        response = parse_request(buf, &request, https);
        if (!response) {
            response = parse_uri(&request, &fileinfo);
            if (!response) {
                // TODO: Handle PARTIAL_CONTENT
                ret = send_file(socket_fd, &request, &fileinfo, ssl);
                if (ret < 0) {
                    status = EXIT_FAILURE;
                    break;
                }
                if (ret > 0) { response = SERVER_ERROR; }
            }
        }
        ret = send_error(socket_fd, &request, response, ssl);
        if (ret < 0) {
            status = EXIT_FAILURE;
            break;
        } else if (ret > 0 && request.http_method == GET) {
            access_log(RESPONSE_CODE[response], buf, fileinfo.size);
        } else {
            access_log(RESPONSE_CODE[response], buf, 0);
        }
        if (!request.connection) {
            close_socket(socket_fd);
            break;
        }
    }
    remove_thread(pthread_self());
    return (void *)(intptr_t)status;  // NOLINT(performance-no-int-to-ptr)
}

void *server(void *arg) {
    int https = (int)(intptr_t)arg;
    int server_sock = 0;
    pthread_t thread = 0;
    SockInfo *sockinfo = NULL;
    SSL_CTX *ctx = NULL;
    SSL *ssl = NULL;
    struct sockaddr_in sockaddr = {.sin_family = AF_INET,
                                   .sin_port = htons(port[https]),
                                   .sin_addr.s_addr = htonl(INADDR_ANY)};

    insert_thread(pthread_self());
    if (https) {
        ctx = init_ssl();
        if (!ctx) {
            if (raise(SIGTERM)) { log_errno(FATAL, "raise", errno); }
            return (void *)EXIT_FAILURE;
        }
    }
    server_sock = init_socket(&sockaddr);
    if (server_sock < 0) {
        if (raise(SIGTERM)) { log_errno(FATAL, "raise", errno); }
        return (void *)EXIT_FAILURE;
    }
    printf("HTTP%s server started\n", https ? "S" : "");

    while (1) {
        if (exit_flag) {
            SSL_CTX_free(ctx);
            close_socket(server_sock);
            break;
        }
        sockinfo = malloc(sizeof(SockInfo));
        if (!sockinfo) {
            log_errno(FATAL, "malloc", errno);
            if (raise(SIGTERM)) { log_errno(FATAL, "raise", errno); }
            close_socket(server_sock);
            break;
        }
        sockinfo->https = https;
        sockinfo->socket_fd = connect_socket(server_sock, &sockinfo->sockaddr);
        if (exit_flag) {
            SSL_CTX_free(ctx);
            free(sockinfo);
            break;
        }
        if (sockinfo->socket_fd < 0) {
            free(sockinfo);
            if (raise(SIGTERM)) { log_errno(FATAL, "raise", errno); }
            return (void *)EXIT_FAILURE;
        }
        if (https) {
            // TODO: Refine SSL
            ssl = connect_ssl(sockinfo->socket_fd, ctx);
            if (!ssl) {
                free(sockinfo);
                continue;
            } else {
                sockinfo->ssl = ssl;
                ssl = NULL;
            }
        }
        pthread_create(&thread, NULL, handle_request, (void *)sockinfo);
    }
    remove_thread(pthread_self());
    return (void *)(intptr_t)EXIT_SUCCESS;
}
