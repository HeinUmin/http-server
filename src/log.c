/**
 * @brief A simple implementation of an Apache-style log system
 */

#include "log.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

const char *LEVEL_STRING[] = {"trace", "debug", "info",
                              "warn",  "error", "fatal"};
const int MAX_LOG_LEVEL = sizeof(LEVEL_STRING) / sizeof(LEVEL_STRING[0]);

static FILE *access_log_file = NULL;
static FILE *error_log_file = NULL;
static long log_level = 0;
static pthread_mutex_t error_log_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t access_log_mutex = PTHREAD_MUTEX_INITIALIZER;

int init_log(long level) {
    log_level = level;
    mkdir("log", 0777);
    access_log_file = fopen("log/access_log", "w");
    error_log_file = fopen("log/error_log", "w");
    if (!access_log_file || !error_log_file) {
        perror("fopen");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int close_log(void) {
    if (fclose(access_log_file) || fclose(error_log_file)) {
        perror("fclose");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int error_log(long level, struct sockaddr_in addr, char *src, char *msg) {
    char time_char[32];
    char ip_str[INET_ADDRSTRLEN];
    time_t now = time(NULL);
    if (level < log_level || level > 5) { return EXIT_SUCCESS; }
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    inet_ntop(AF_INET, &addr.sin_addr, ip_str, INET_ADDRSTRLEN);
    if (strftime(time_char, sizeof(time_char), "%c", &timeinfo) == 0) {
        perror("strftime");
        return EXIT_FAILURE;
    }
    pthread_mutex_lock(&error_log_mutex);
    if (fprintf(error_log_file, "[%s] [%s] [pid %d] [client %s:%d] %s:  %s\n",
                time_char, LEVEL_STRING[level], getpid(), ip_str,
                ntohs(addr.sin_port), src, msg) < 0) {
        perror("fprintf");
        pthread_mutex_unlock(&error_log_mutex);
        return EXIT_FAILURE;
    }
    pthread_mutex_unlock(&error_log_mutex);
    return EXIT_SUCCESS;
}

int access_log(struct sockaddr_in addr, char *request, int code, ssize_t sent) {
    int err = 0;
    char time_char[32];
    time_t now = time(NULL);
    struct tm timeinfo;
    char ip_str[INET_ADDRSTRLEN];
    gmtime_r(&now, &timeinfo);
    if (strftime(time_char, sizeof(time_char), "[%d/%b/%Y:%H:%M:%S %z]",
                 &timeinfo) == 0) {
        return error_log(4, addr, "access_log", "strftime failed");
    }
    inet_ntop(AF_INET, &addr.sin_addr, ip_str, INET_ADDRSTRLEN);
    pthread_mutex_lock(&access_log_mutex);
    err += fprintf(access_log_file, "%s - - %s ", ip_str, time_char) < 0;
    err += fprintf(access_log_file, "\"%s\" ", request) < 0;
    err += fprintf(access_log_file, "%d %ld\n", code, sent) < 0;
    pthread_mutex_unlock(&access_log_mutex);
    if (err) { return error_log(4, addr, "access_log", "write log failed"); }
    return EXIT_SUCCESS;
}

int flush_error_log(void) {
    pthread_mutex_lock(&error_log_mutex);
    int ret = fflush(error_log_file);
    pthread_mutex_unlock(&error_log_mutex);
    return ret;
}

int flush_access_log(void) {
    pthread_mutex_lock(&access_log_mutex);
    int ret = fflush(access_log_file);
    pthread_mutex_unlock(&access_log_mutex);
    return ret;
}
