#include "server.h"
#include "log.h"
#include "parser.h"
#include "sighandler.h"
#include "socket.h"
#include "utils.h"

#include <errno.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

static inline int get_request_line(char *buf, const Request *request) {
    if (!buf || !request) { return EXIT_FAILURE; }
    if (sprintf(buf, "%s %s HTTP/1.%d",
                HTTP_METHOD_STRING[request->http_method], request->http_uri,
                request->http_version) < 0) {
        log_errno(ERROR, "sprintf", errno);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

static int send_error(int socket_fd,
                      const Request *request,
                      Response response,
                      long filesize,
                      SSL *ssl) {
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
                        "Location: https://%s", request->http_host) < 0;
        if (port[1] != 443) {
            err |= snprintf(buf + strlen(buf), 8, ":%hu", port[1]) < 0;
        }
        err |= snprintf(buf + strlen(buf), HTTP_BUF_SIZE - strlen(buf),
                        "%s\r\n", request->http_uri) < 0;
    }
    if (response == RANGE_UNSAT && filesize > 0) {
        err |= snprintf(buf + strlen(buf), HTTP_BUF_SIZE - strlen(buf),
                        "Content-Range: bytes */%zu\r\n", filesize) < 0;
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
    if (get_request_line(buf, request)) { buf[0] = '\0'; }
    access_log(RESPONSE_CODE[response], buf, 0);
    return 0;
}

static long send_file(int socket_fd,
                      const Request *request,
                      const FileInfo *fileinfo,
                      SSL *ssl) {
    FILE *fp = NULL;
    int response = 200;
    long status = 0, start = 0, end = 0, rest = 0;
    size_t readret = 0;
    ssize_t sendret = 0;
    char buf[HTTP_BUF_SIZE];
    char time_str[TIMESTRLEN];

    memset(buf, 0, HTTP_BUF_SIZE);
    get_time(time_str, "%a, %d %b %Y %H:%M:%S GMT", 1);
    if (request->range_start >= fileinfo->size) { return -3; }
    if (request->range_start < request->range_end) {
        start = request->range_start;
        end = request->range_end < fileinfo->size ? request->range_end :
                                                    fileinfo->size - 1;
        status |=
            snprintf(buf, HTTP_BUF_SIZE, "HTTP/1.%d 206 Partial Content\r\n",
                     request->http_version) < 0;
        response = 206;
    } else {
        start = request->range_start, end = fileinfo->size - 1;
        status |= snprintf(buf, HTTP_BUF_SIZE, "HTTP/1.%d 200 OK\r\n",
                           request->http_version) < 0;
    }
    status |= snprintf(buf + strlen(buf), HTTP_BUF_SIZE - strlen(buf),
                       "Connection: %s\r\n",
                       request->connection ? "keep-alive" : "close") < 0;
    status |= snprintf(buf + strlen(buf), HTTP_BUF_SIZE - strlen(buf),
                       "Content-Type: %s\r\n", fileinfo->type) < 0;
    status |= snprintf(buf + strlen(buf), HTTP_BUF_SIZE - strlen(buf),
                       "Content-Length: %zu\r\n", end - start + 1) < 0;
    status |= snprintf(buf + strlen(buf), HTTP_BUF_SIZE - strlen(buf),
                       "Content-Location: %s\r\n", request->http_uri) < 0;
    status |= snprintf(buf + strlen(buf), HTTP_BUF_SIZE - strlen(buf),
                       "Content-Range: bytes %zu-%zu/%zu\r\n", start, end,
                       fileinfo->size) < 0;
    status |= snprintf(buf + strlen(buf), HTTP_BUF_SIZE - strlen(buf),
                       "Last-Modified: %s\r\n", fileinfo->time) < 0;
    status |= snprintf(buf + strlen(buf), HTTP_BUF_SIZE - strlen(buf),
                       "Accept-Ranges: bytes\r\n") < 0;
    status |= snprintf(buf + strlen(buf), HTTP_BUF_SIZE - strlen(buf),
                       "Date: %s\r\n\r\n", time_str) < 0;
    if (status) {
        log_errno(ERROR, "snprintf", errno);
        return -2;
    }

    // Send header
    if (strlen(buf) >= HTTP_BUF_SIZE - 1) {
        loge("send_file", "Buffer overflow");
        return -2;
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

    // Send file
    if (fseek(fp, start, SEEK_SET)) {
        log_errno(ERROR, "fseek", errno);
        close_socket(socket_fd);
        return -1;
    }
    rest = end - start + 1;
    while (rest > 0) {
        memset(buf, 0, HTTP_BUF_SIZE);
        readret = fread(buf, sizeof(char),
                        rest > HTTP_BUF_SIZE ? HTTP_BUF_SIZE : rest, fp);
        if (ferror(fp)) {
            log_errno(ERROR, "fread", errno);
            close_socket(socket_fd);
            break;
        }
        sendret = send_message(socket_fd, buf, (int)readret, ssl);
        if (sendret < 0) { break; }
        status += sendret;
        rest -= sendret;
    }

    if (get_request_line(buf, request)) { buf[0] = '\0'; }
    access_log(response, buf, status);
    if (rest) {
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
    long ret = 0;
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
                ret = send_file(socket_fd, &request, &fileinfo, ssl);
                if (ret == -1) {
                    break;
                } else if (ret == -2) {
                    response = SERVER_ERROR;
                } else if (ret == -3) {
                    response = RANGE_UNSAT;
                }
            }
        }
        if (send_error(socket_fd, &request, response, fileinfo.size, ssl) < 0) {
            break;
        }
        if (!request.connection) {
            close_socket(socket_fd);
            break;
        }
    }
    close_ssl(ssl, socket_fd);
    remove_thread(pthread_self());
    return NULL;
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
            if (kill(getpid(), SIGTERM)) { log_errno(FATAL, "raise", errno); }
            return (void *)EXIT_FAILURE;
        }
    }
    server_sock = init_socket(&sockaddr);
    if (server_sock < 0) {
        if (kill(getpid(), SIGTERM)) { log_errno(FATAL, "raise", errno); }
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
            if (kill(getpid(), SIGTERM)) { log_errno(FATAL, "raise", errno); }
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
            if (kill(getpid(), SIGTERM)) { log_errno(FATAL, "raise", errno); }
            return (void *)EXIT_FAILURE;
        }
        if (https) {
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
