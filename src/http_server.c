#include "http_server.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

void *http_server(void *arg) {
    printf("HTTP server started\n");
    // TODO: Implement http server

    return (void *)EXIT_SUCCESS;
}
