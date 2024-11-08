/**
 * @brief A simple implementation of an Apache-style log system
 */

#include "log.h"
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define ERR_BUF_SIZE 64
#define TIME_BUF_SIZE 32
#define access_log_error()                                                     \
    {                                                                          \
        strerror_r(errno, err_buf, ERR_BUF_SIZE);                              \
        error_log(ERROR, addr, "access_log", err_buf);                         \
        return EXIT_FAILURE;                                                   \
    }

const char *LEVEL_STRING[] = {"trace", "debug", "info",
                              "warn",  "error", "fatal"};

static FILE *access_log_file = NULL;
static FILE *error_log_file = NULL;
static long log_level = 0;
static pthread_mutex_t error_log_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t access_log_mutex = PTHREAD_MUTEX_INITIALIZER;
static const struct sockaddr_in empty_socket = {0};

int init_log(long level) {
    if (level >= NR_LOG_LEVEL || level < 0) { return EXIT_FAILURE; }
    log_level = level;
    mkdir("log", 0755);
    access_log_file = fopen("log/access_log", "w");
    error_log_file = fopen("log/error_log", "w");
    if (!access_log_file || !error_log_file) {
        perror("fopen");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int close_log(void) {
    if ((access_log_file != stdout && fclose(access_log_file)) ||
        fclose(error_log_file)) {
        perror("fclose");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int error_log(long level,
              const struct sockaddr_in *addr,
              char *src,
              char *msg) {
    char time_buf[TIME_BUF_SIZE];
    char ip_str[INET_ADDRSTRLEN];
    struct tm timeinfo;
    time_t now = time(NULL);

    if (level >= NR_LOG_LEVEL || level < 0) {
        error_log(ERROR, addr, "error_log", "Invalid log level");
        return EXIT_FAILURE;
    }
    if (level < log_level) { return EXIT_SUCCESS; }
    if (!addr) { addr = &empty_socket; }
    if (!src || !msg) {
        error_log(ERROR, addr, "error_log", "Invalid log source or message");
        return EXIT_FAILURE;
    }

    localtime_r(&now, &timeinfo);
    inet_ntop(AF_INET, &(*addr).sin_addr, ip_str, INET_ADDRSTRLEN);
    if (strftime(time_buf, TIME_BUF_SIZE, "%c", &timeinfo) == 0) {
        perror("strftime");
        return EXIT_FAILURE;
    }

    if (pthread_mutex_lock(&error_log_mutex)) {
        perror("pthread_mutex_lock");
        return EXIT_FAILURE;
    }
    if (fprintf(error_log_file, "[%s] [%s] [pid %d] [client %s:%d] %s:  %s\n",
                time_buf, LEVEL_STRING[level], getpid(), ip_str,
                ntohs((*addr).sin_port), src, msg) < 0) {
        perror("fprintf");
    }
    if (fflush(error_log_file)) {
        perror("fflush");
        error_log_file = stdout;
        pthread_mutex_unlock(&error_log_mutex);
        error_log(WARN, addr, "error_log",
                  "Error log has been redirected to stdout");
        return EXIT_FAILURE;
    }
    if (pthread_mutex_unlock(&error_log_mutex)) {
        perror("pthread_mutex_unlock");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int access_log(int code,
               const struct sockaddr_in *addr,
               char *request,
               ssize_t sent) {
    char err_buf[ERR_BUF_SIZE];
    char time_buf[TIME_BUF_SIZE];
    struct tm timeinfo;
    char ip_str[INET_ADDRSTRLEN];
    time_t now = time(NULL);

    if (!addr) { addr = &empty_socket; }
    if (!request) { request = ""; }

    inet_ntop(AF_INET, &(*addr).sin_addr, ip_str, INET_ADDRSTRLEN);
    gmtime_r(&now, &timeinfo);
    if (strftime(time_buf, TIME_BUF_SIZE, "[%d/%b/%Y:%H:%M:%S %z]",
                 &timeinfo) == 0) {
        access_log_error();
    }

    if (pthread_mutex_lock(&access_log_mutex)) { access_log_error(); }
    if (fprintf(access_log_file, "%s - - %s \"%s\" %d %ld\n", ip_str, time_buf,
                request, code, sent) < 0) {
        pthread_mutex_unlock(&access_log_mutex);
        access_log_error();
    }
    if (fflush(access_log_file)) {
        pthread_mutex_unlock(&access_log_mutex);
        access_log_error();
    }
    if (pthread_mutex_unlock(&access_log_mutex)) { access_log_error(); }
    return EXIT_SUCCESS;
}
