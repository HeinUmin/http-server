/**
 * @brief A simple implementation of an Apache-style log system
 */

#include "log.h"
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define ERR_BUF_SIZE 64
#define TIME_BUF_SIZE 32

static FILE *access_log_file = NULL;
static FILE *error_log_file = NULL;
static long log_level = 0;
static pthread_mutex_t error_log_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t access_log_mutex = PTHREAD_MUTEX_INITIALIZER;
static const struct sockaddr_in empty_socket = {0};
const char *LEVEL_STRING[] = {"trace", "debug", "info",
                              "warn",  "error", "fatal"};

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
              const char *src,
              const char *msg) {
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

    pthread_mutex_lock(&error_log_mutex);
    if (fprintf(error_log_file, "[%s] [%s] [pid %d] [client %s:%d] %s:  %s\n",
                time_buf, LEVEL_STRING[level], getpid(), ip_str,
                ntohs((*addr).sin_port), src, msg) < 0) {
        perror("fprintf");
        pthread_mutex_unlock(&error_log_mutex);
        return EXIT_FAILURE;
    }
    if (fflush(error_log_file)) {
        perror("fflush");
        error_log_file = stdout;
        pthread_mutex_unlock(&error_log_mutex);
        error_log(WARN, addr, "error_log",
                  "Error log has been redirected to stdout");
        return EXIT_FAILURE;
    }
    pthread_mutex_unlock(&error_log_mutex);
    return EXIT_SUCCESS;
}

int access_log(int code,
               const struct sockaddr_in *addr,
               const char *request,
               ssize_t sent) {
    char ip_str[INET_ADDRSTRLEN];
    char time_buf[TIME_BUF_SIZE];
    struct tm timeinfo;
    time_t now = time(NULL);

    if (!addr) { addr = &empty_socket; }
    if (!request) { request = ""; }

    inet_ntop(AF_INET, &(*addr).sin_addr, ip_str, INET_ADDRSTRLEN);
    gmtime_r(&now, &timeinfo);
    if (strftime(time_buf, TIME_BUF_SIZE, "[%d/%b/%Y:%H:%M:%S %z]",
                 &timeinfo) == 0) {
        log_errno(ERROR, addr, "strftime", errno);
        return EXIT_FAILURE;
    }

    pthread_mutex_lock(&access_log_mutex);
    if (fprintf(access_log_file, "%s - - %s \"%s\" %d %ld\n", ip_str, time_buf,
                request, code, sent) < 0) {
        pthread_mutex_unlock(&access_log_mutex);
        log_errno(ERROR, addr, "access_log", errno);
        return EXIT_FAILURE;
    }
    if (fflush(access_log_file)) {
        pthread_mutex_unlock(&access_log_mutex);
        log_errno(ERROR, addr, "access_log", errno);
        return EXIT_FAILURE;
    }
    pthread_mutex_unlock(&access_log_mutex);
    return EXIT_SUCCESS;
}

void log_errno(int level,
               const struct sockaddr_in *addr,
               const char *src,
               int errnum) {
    char err_buf[ERR_BUF_SIZE];
    strerror_r(errnum, err_buf, ERR_BUF_SIZE);
    error_log(level, addr, src, err_buf);
}

void log_format(int level,
                const struct sockaddr_in *addr,
                const char *src,
                const char *format,
                ...) {
    char err_buf[ERR_BUF_SIZE];
    va_list args;
    va_start(args, format);
    if (vsnprintf(err_buf, ERR_BUF_SIZE, format, args) < 0) {
        log_errno(WARN, addr, src, errno);
    } else {
        error_log(level, addr, src, err_buf);
    }
    va_end(args);
}
