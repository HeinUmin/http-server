/**
 * @brief A simple implementation of an Apache-style log system
 */

#include "log.h"
#include "utils.h"

#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define ERR_BUF_SIZE 64

static FILE *access_log_file = NULL;
static FILE *error_log_file = NULL;
static LogLevel log_level = 0;
static pthread_mutex_t error_log_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t access_log_mutex = PTHREAD_MUTEX_INITIALIZER;
const char *LEVEL_STRING[] = {"trace", "debug", "info",
                              "warn",  "error", "fatal"};

int init_log(LogLevel level) {
    if ((level >= NR_LOG_LEVEL || level < 0) ||
        (access_log_file || error_log_file)) {
        return EXIT_FAILURE;
    }
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
    if (!access_log_file || !error_log_file) { return EXIT_FAILURE; }
    if ((access_log_file != stdout && fclose(access_log_file)) ||
        fclose(error_log_file)) {
        perror("fclose");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int error_log(LogLevel level, const char *src, const char *msg) {
    char time_str[TIMESTRLEN];
    char ip_str[INET_ADDRSTRLEN];

    if (level >= NR_LOG_LEVEL || level < 0) {
        error_log(ERROR, "error_log", "Invalid log level");
        return EXIT_FAILURE;
    }
    if (level < log_level) { return EXIT_SUCCESS; }
    if (!src || !msg) {
        error_log(ERROR, "error_log", "Invalid log source or message");
        return EXIT_FAILURE;
    }
    get_time(time_str, "%c");
    inet_ntop(AF_INET, &sock.sin_addr, ip_str, INET_ADDRSTRLEN);

    pthread_mutex_lock(&error_log_mutex);
    if (fprintf(error_log_file, "[%s] [%s] [pid %d:tid %lu] [%s:%d] %s:  %s\n",
                time_str, LEVEL_STRING[level], getpid(), pthread_self(), ip_str,
                ntohs(sock.sin_port), src, msg) < 0 ||
        fflush(error_log_file)) {
        perror("error_log");
        error_log_file = stdout;
        pthread_mutex_unlock(&error_log_mutex);
        error_log(WARN, "error_log", "Error log redirected to stdout");
        return EXIT_FAILURE;
    }
    pthread_mutex_unlock(&error_log_mutex);
    return EXIT_SUCCESS;
}

int access_log(int code, const char *request, ssize_t sent) {
    char ip_str[INET_ADDRSTRLEN];
    char time_str[TIMESTRLEN];

    if (!request) { request = ""; }
    get_time(time_str, "%d/%b/%Y:%H:%M:%S %z");
    inet_ntop(AF_INET, &sock.sin_addr, ip_str, INET_ADDRSTRLEN);

    pthread_mutex_lock(&access_log_mutex);
    if (fprintf(access_log_file, "%s - - [%s] \"%s\" %d %ld\n", ip_str,
                time_str, request, code, sent) < 0 ||
        fflush(access_log_file)) {
        pthread_mutex_unlock(&access_log_mutex);
        log_errno(ERROR, "access_log", errno);
        return EXIT_FAILURE;
    }
    pthread_mutex_unlock(&access_log_mutex);
    return EXIT_SUCCESS;
}

void log_errno(LogLevel level, const char *src, int errnum) {
    char err_buf[ERR_BUF_SIZE];
    strerror_r(errnum, err_buf, ERR_BUF_SIZE);
    error_log(level, src, err_buf);
}

void log_format(int level, const char *src, const char *format, ...) {
    char err_buf[ERR_BUF_SIZE];
    va_list args;
    va_start(args, format);
    if (vsnprintf(err_buf, ERR_BUF_SIZE, format, args) < 0) {
        log_errno(WARN, src, errno);
    } else {
        error_log(level, src, err_buf);
    }
    va_end(args);
}
