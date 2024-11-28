#include "log.h"
#include "server.h"
#include "sighandler.h"
#include "utils.h"

#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <string.h>

void usage(char *name) {
    int i = 0;
    printf("Usage: %s [options]\n", name);
    printf("Options:\n");
    printf("  -h, --help               Display this help message\n");
    printf("  -p, --http-port=PORT     Set HTTP port, 0 for close\n");
    printf("  -s, --https-port=PORT    Set HTTPS port, 0 for close\n");
    printf("  -l, --log-level={");
    for (i = 0; i < NR_LOG_LEVEL; i++) {
        printf("%s%s", LEVEL_STRING[i], (i == NR_LOG_LEVEL - 1) ? "}\n" : "|");
    }
    printf("                           Set log level\n");
}

long get_port(char *port_str) {
    char *temp_ptr = NULL;
    long portno = strtol(port_str, &temp_ptr, 10);
    if (temp_ptr[0]) {
        perror("strtol");
        return -1;
    }
    if (portno < 0 || portno >= 65536) {
        errno = EINVAL;
        perror("getopt");
        return -1;
    }
    return portno;
}

int main(int argc, char *argv[]) {
    sigset_t sigset;
    LogLevel log_level = INFO;
    pthread_t handler_thread = 0, http_thread = 0, https_thread = 0;
    void *handler_status = NULL, *http_status = NULL, *https_status = NULL;

    // Parse args
    int opt = 0;
    long temp = 0;
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
            temp = get_port(optarg);
            if (temp < 0) {
                usage(argv[0]);
                return EXIT_FAILURE;
            }
            port[0] = temp;
            break;
        case 's':
            temp = get_port(optarg);
            if (temp < 0) {
                usage(argv[0]);
                return EXIT_FAILURE;
            }
            port[1] = temp;
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

    // Setup signal handler
    if (init_log(log_level)) { return EXIT_FAILURE; }
    sigemptyset(&sigset), sigaddset(&sigset, SIGQUIT),
        sigaddset(&sigset, SIGTERM), sigaddset(&sigset, SIGINT);
    pthread_sigmask(SIG_BLOCK, &sigset, NULL);

    // Start server
    pthread_create(&handler_thread, NULL, signal_thread, &sigset);
    if (port[0]) { pthread_create(&http_thread, NULL, server, (void *)0); }
    if (port[1]) { pthread_create(&https_thread, NULL, server, (void *)1); }

    // Wait for server to finish
    pthread_join(handler_thread, &handler_status);
    pthread_sigmask(SIG_UNBLOCK, &sigset, NULL);
    if (port[0]) { pthread_join(http_thread, &http_status); }
    if (port[1]) { pthread_join(https_thread, &https_status); }

    return close_log() || handler_thread || http_status || https_status;
}
