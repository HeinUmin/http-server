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

void usage(char *name) {
    int i = 0;
    printf("Usage: %s [options]\n", name);
    printf("Options:\n");
    printf("  -l, --log-level={");
    for (i = 0; i < NR_LOG_LEVEL; i++) {
        printf("%s%s", LEVEL_STRING[i], (i == NR_LOG_LEVEL - 1) ? "}\n" : "|");
    }
    printf("                           Set log level\n");
    printf("  -h, --help               Display this help message\n");
}

uint16_t get_port(char *port_str) {
    char *temp_ptr = NULL;
    long port = strtol(port_str, &temp_ptr, 10);
    if (temp_ptr[0]) {
        perror("strtol");
        return -1;
    }
    if (port < 0 || port >= 65536) {
        errno = EINVAL;
        perror("getopt");
        return -1;
    }
    return port;
}

int main(int argc, char *argv[]) {
    sigset_t sigset;
    LogLevel log_level = INFO;
    uint16_t http_port = 80, https_port = 443;
    pthread_t handler_thread = 0, http_thread = 0, https_thread = 0;
    void *handler_status = NULL, *http_status = NULL, *https_status = NULL;

    int opt = 0, temp = 0;
    struct option longopts[] = {{"help", no_argument, NULL, 'h'},
                                {"http-port", required_argument, NULL, 'p'},
                                {"https-port", required_argument, NULL, 's'},
                                {"log-level", required_argument, NULL, 'l'}};
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    while ((opt = getopt_long(argc, argv, "hp:s:l:", longopts, NULL)) != -1) {
        switch (opt) {
        case 'h':
            usage(argv[0]);
            return EXIT_SUCCESS;
        case 'p':
            http_port = get_port(optarg);
            if (http_port < 0) {
                usage(argv[0]);
                return EXIT_FAILURE;
            }
            break;
        case 's':
            https_port = get_port(optarg);
            if (https_port < 0) {
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
                perror("getopt");
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
    sigemptyset(&sigset), sigaddset(&sigset, SIGQUIT),
        sigaddset(&sigset, SIGTERM), sigaddset(&sigset, SIGINT);
    pthread_sigmask(SIG_BLOCK, &sigset, NULL);

    pthread_create(&handler_thread, NULL, signal_thread, &sigset);
    pthread_create(&http_thread, NULL, http_server, &http_port);
    pthread_create(&https_thread, NULL, https_server, &https_port);

    pthread_join(handler_thread, &handler_status);
    pthread_sigmask(SIG_UNBLOCK, &sigset, NULL);
    pthread_join(http_thread, &http_status);
    pthread_join(https_thread, &https_status);

    return close_log() || handler_thread || http_status || https_status;
}
