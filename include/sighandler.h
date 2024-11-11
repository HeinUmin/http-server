#ifndef SIGHANDLER_H
#define SIGHANDLER_H

#include <pthread.h>

typedef struct tagThreadPoll {
    pthread_t thread;
    struct tagThreadPoll *next;
} ThreadPoll;

extern volatile int exit_flag;

void insert_thread(pthread_t thread);
void remove_thread(pthread_t thread);
void signal_handler(int sig);
void *signal_thread(void *arg);

#endif
