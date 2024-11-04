#include "http_server.h"
#include "https_server.h"
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    pthread_t http_thread = 0;
    pthread_t https_thread = 0;
    void *http_status = NULL;
    void *https_status = NULL;

    // TODO: Parse arguments

    pthread_create(&http_thread, NULL, http_server, NULL);
    pthread_create(&https_thread, NULL, https_server, NULL);

    pthread_join(http_thread, &http_status);
    pthread_join(https_thread, &https_status);

    return (int)(intptr_t)http_status || (int)(intptr_t)https_status;
}
