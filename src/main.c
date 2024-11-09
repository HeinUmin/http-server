#include "log.h"
#include "server.h"
#include "sighandler.h"

#include <errno.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define check_error(err, src)                                                  \
    do {                                                                       \
        if (err) {                                                             \
            error_log(NR_LOG_LEVEL - 1, NULL, src, strerror(err));             \
            close_log();                                                       \
            return EXIT_FAILURE;                                               \
        }                                                                      \
    } while (0)

void usage(char *name) {
    printf("Usage: %s [options]\n", name);
    printf("Options:\n");
    printf("  -l, --log-level={");
    for (int i = 0; i < NR_LOG_LEVEL; i++) {
        printf("%s%s", LEVEL_STRING[i], (i == NR_LOG_LEVEL - 1) ? "}\n" : "|");
    }
    printf("                           Set log level\n");
    printf("  -h, --help               Display this help message\n");
}

// NOLINTBEGIN(concurrency-mt-unsafe)
int main(int argc, char *argv[]) {
    int temp = 0;
    long log_level = 2;
    long http_port = 80;
    long https_port = 443;
    pthread_t handler_thread = 0;
    pthread_t http_thread = 0;
    pthread_t https_thread = 0;
    char *temp_ptr = NULL;
    void *handler_status = NULL;
    void *http_status = NULL;
    void *https_status = NULL;
    sigset_t sigset;

    int opt = 0;
    struct option long_options[] = {
        {"help", no_argument, NULL, 'h'},
        {"http-port", required_argument, NULL, 'p'},
        {"https-port", required_argument, NULL, 's'},
        {"log-level", required_argument, NULL, 'l'}};
    while ((opt = getopt_long(argc, argv, "hp:s:l:", long_options, NULL)) !=
           -1) {
        switch (opt) {
        case 'h':
            usage(argv[0]);
            return EXIT_SUCCESS;
        case 'p':
            http_port = strtol(optarg, &temp_ptr, 10);
            if (temp_ptr[0] != '\0') {
                perror("strtol");
                usage(argv[0]);
                return EXIT_FAILURE;
            }
            if (http_port < 0 || http_port > 65535) {
                errno = EINVAL;
                perror("getopt_long");
                usage(argv[0]);
                return EXIT_FAILURE;
            }
            break;
        case 's':
            https_port = strtol(optarg, &temp_ptr, 10);
            if (temp_ptr[0] != '\0') {
                perror("strtol");
                usage(argv[0]);
                return EXIT_FAILURE;
            }
            if (https_port < 0 || https_port > 65535) {
                errno = EINVAL;
                perror("getopt_long");
                usage(argv[0]);
                return EXIT_FAILURE;
            }
            break;
        case 'l':
            for (temp = 0; temp < NR_LOG_LEVEL; temp++) {
                if (strcasecmp(optarg, LEVEL_STRING[temp]) == 0) {
                    log_level = temp;
                    break;
                }
            }
            if (temp == NR_LOG_LEVEL) {
                errno = EINVAL;
                perror("getopt_long");
                usage(argv[0]);
                return EXIT_FAILURE;
            }
            break;
        default:
            usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (init_log(log_level)) { return EXIT_FAILURE; }
    if (sigemptyset(&sigset) || sigaddset(&sigset, SIGQUIT) ||
        sigaddset(&sigset, SIGTERM) || sigaddset(&sigset, SIGINT)) {
        check_error(errno, "sigaddset");
    }
    temp = pthread_sigmask(SIG_BLOCK, &sigset, NULL);
    check_error(temp, "pthread_sigmask");

    temp = pthread_create(&handler_thread, NULL, signal_thread, &sigset);
    check_error(temp, "pthread_create");
    temp = pthread_create(&http_thread, NULL, http_server, &http_port);
    check_error(temp, "pthread_create");
    insert_thread(http_thread);
    temp = pthread_create(&https_thread, NULL, https_server, &https_port);
    check_error(temp, "pthread_create");
    insert_thread(https_thread);

    temp = pthread_join(handler_thread, &handler_status);
    check_error(temp, "pthread_join");
    if (handler_status) { return EXIT_FAILURE; }
    temp = pthread_sigmask(SIG_UNBLOCK, &sigset, NULL);
    check_error(temp, "pthread_sigmask");

    temp = pthread_join(http_thread, &http_status);
    check_error(temp, "pthread_join");
    remove_thread(http_thread);
    temp = pthread_join(https_thread, &https_status);
    check_error(temp, "pthread_join");
    remove_thread(https_thread);

    return close_log() || http_status || https_status;
}
// NOLINTEND(concurrency-mt-unsafe)
