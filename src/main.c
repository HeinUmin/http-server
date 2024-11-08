#include "log.h"
#include "server.h"

#include <errno.h>
#include <getopt.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define check_error(temp, src)                                                 \
    if (temp) {                                                                \
        error_log(NR_LOG_LEVEL - 1, NULL, src, strerror(temp));                \
        close_log();                                                           \
        return EXIT_FAILURE;                                                   \
    }

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
    pthread_t http_thread = 0;
    pthread_t https_thread = 0;
    void *http_status = NULL;
    void *https_status = NULL;

    int opt = 0;
    struct option long_options[] = {
        {"help", no_argument, NULL, 'h'},
        {"log-level", required_argument, NULL, 'l'}
        // TODO: Parse more arguments
    };
    while ((opt = getopt_long(argc, argv, "hl:", long_options, NULL)) != -1) {
        switch (opt) {
        case 'h':
            usage(argv[0]);
            return EXIT_SUCCESS;
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

    temp = pthread_create(&http_thread, NULL, http_server, NULL);
    check_error(temp, "pthread_create");
    temp = pthread_create(&https_thread, NULL, https_server, NULL);
    check_error(temp, "pthread_create");

    temp = pthread_join(http_thread, &http_status);
    check_error(temp, "pthread_join");
    temp = pthread_join(https_thread, &https_status);
    check_error(temp, "pthread_join");

    return close_log() || http_status || https_status;
}
// NOLINTEND(concurrency-mt-unsafe)
