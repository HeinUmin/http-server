#include "socket.h"
#include "log.h"
#include "utils.h"

#include <errno.h>
#include <openssl/err.h>
#include <string.h>
#include <unistd.h>

int close_socket(int socket_fd) {
    int ret = close(socket_fd);
    if (ret) {
        log_errno(FATAL, "close", errno);
        return ret;
    }
    logi("close_socket", "Close socket %d", socket_fd);
    return 0;
}

int init_socket(struct sockaddr_in *sockaddr) {
    int reuse = 1;

    // Get socket address
    int socket_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        log_errno(FATAL, "socket", errno);
        return -1;
    }
    logi("init_socket", "Init socket %d", socket_fd);

    // Set socket reusable
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) <
        0) {
        log_errno(WARN, "setsockopt", errno);
    } else {
        logi("init_socket", "Set reusable to socket %d", socket_fd);
    }

    // Bind socket
    if (sockaddr) {
        if (bind(socket_fd, (struct sockaddr *)sockaddr,
                 sizeof(struct sockaddr_in))) {
            log_errno(FATAL, "bind", errno);
            close_socket(socket_fd);
            return -1;
        }
        sock = *sockaddr;
        logi("init_socket", "Bind socket %d", socket_fd);
    }

    // Listen socket
    if (listen(socket_fd, 5)) {
        log_errno(FATAL, "listen", errno);
        close_socket(socket_fd);
        return -1;
    }
    logi("init_socket", "Listen socket %d", socket_fd);
    return socket_fd;
}

int connect_socket(int socket_fd, struct sockaddr_in *sockaddr) {
    socklen_t len = sizeof(struct sockaddr_in);
    int client_sock = accept(socket_fd, (struct sockaddr *)sockaddr, &len);
    while (errno == EAGAIN) {
        logw("connect_socket", "Retry accept");
        client_sock = accept(socket_fd, (struct sockaddr *)sockaddr, &len);
    }
    if (client_sock < 0) {
        log_errno(ERROR, "accept", errno);
        return -1;
    }
    logd("connect_socket", "Accept socket %d", client_sock);
    return client_sock;
}

ssize_t recv_message(int socket_fd, char *buf, int len, SSL *ssl) {
    ssize_t ret = 0;
    memset(buf, 0, len);
    if (ssl) {
        ret = SSL_read(ssl, buf, len - 1);
    } else {
        ret = recv(socket_fd, buf, len - 1, 0);
    }
    if (ret < 0) {
        log_errno(WARN, "recv", errno);
        close_socket(socket_fd);
    } else if (ret == 0) {
        close_socket(socket_fd);
    } else {
        error_log(TRACE, "recv_msg", buf);
    }
    return ret;
}

ssize_t send_message(int socket_fd, const char *buf, int len, SSL *ssl) {
    ssize_t ret = 0;
    if (ssl) {
        ret = SSL_write(ssl, buf, len);
    } else {
        ret = send(socket_fd, buf, len, 0);
    }
    if (ret < 0) {
        log_errno(WARN, "send", errno);
        close_socket(socket_fd);
    } else if (ret == 0) {
        close_socket(socket_fd);
    } else {
        error_log(TRACE, "send_msg", buf);
    }
    return ret;
}

SSL_CTX *init_ssl(void) {
    char err_buf[128];
    SSL_CTX *ctx = NULL;
    // Initialize SSL
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    // Create SSL context
    ctx = SSL_CTX_new(SSLv23_server_method());
    if (ctx == NULL) {
        logf("init_ssl", "%s", ERR_error_string(ERR_get_error(), err_buf));
        return NULL;
    }
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
    // Load keys
    if (SSL_CTX_use_certificate_file(ctx, "keys/cnlab.cert",
                                     SSL_FILETYPE_PEM) <= 0) {
        logf("init_ssl", "%s", ERR_error_string(ERR_get_error(), err_buf));
        SSL_CTX_free(ctx);
        return NULL;
    }
    if (SSL_CTX_use_PrivateKey_file(ctx, "keys/cnlab.prikey",
                                    SSL_FILETYPE_PEM) <= 0) {
        logf("init_ssl", "%s", ERR_error_string(ERR_get_error(), err_buf));
        SSL_CTX_free(ctx);
        return NULL;
    }
    if (!SSL_CTX_check_private_key(ctx)) {
        logf("init_ssl", "%s", ERR_error_string(ERR_get_error(), err_buf));
        SSL_CTX_free(ctx);
        return NULL;
    }
    return ctx;
}

int close_ssl(SSL *ssl, int socket_fd) {
    char err_buf[128];
    if (SSL_shutdown(ssl) < 0) {
        loge("close_ssl", "%s", ERR_error_string(ERR_get_error(), err_buf));
        close_socket(socket_fd);
        return -1;
    }
    SSL_free(ssl);
    return 0;
}

SSL *connect_ssl(int socket_fd, SSL_CTX *ctx) {
    char err_buf[128];
    SSL *ssl = SSL_new(ctx);
    if (!ssl) {
        loge("connect_ssl", "%s", ERR_error_string(ERR_get_error(), err_buf));
        close_socket(socket_fd);
        return NULL;
    }
    SSL_set_fd(ssl, socket_fd);
    if (SSL_accept(ssl) < 0) {
        loge("connect_ssl", "%s", ERR_error_string(ERR_get_error(), err_buf));
        SSL_free(ssl);
        close_socket(socket_fd);
        return NULL;
    }
    return ssl;
}
