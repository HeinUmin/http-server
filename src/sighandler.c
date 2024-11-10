#include "sighandler.h"
#include "log.h"

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

volatile int exit_flag = 0;
struct Thread_poll *thread_poll = NULL;

void close_threads(void) {
    struct Thread_poll *cur_thread = NULL;
    exit_flag++;
    while (thread_poll) {
        cur_thread = thread_poll;
        pthread_kill(cur_thread->thread, SIGUSR1);
        thread_poll = thread_poll->next;
        free(cur_thread);
    }
}

void insert_thread(pthread_t thread) {
    struct Thread_poll *new_thread = malloc(sizeof(struct Thread_poll));
    if (!new_thread) {
        log_errno(FATAL, NULL, "malloc", errno);
        close_threads();
        return;
    }
    new_thread->thread = thread;
    new_thread->next = thread_poll;
    thread_poll = new_thread;
}

void remove_thread(pthread_t thread) {
    struct Thread_poll *cur_thread = thread_poll;
    struct Thread_poll *prev_thread = NULL;
    while (cur_thread) {
        if (cur_thread->thread == thread) {
            if (prev_thread) {
                prev_thread->next = cur_thread->next;
            } else {
                thread_poll = cur_thread->next;
            }
            free(cur_thread);
            return;
        }
        prev_thread = cur_thread;
        cur_thread = cur_thread->next;
    }
}

void signal_handler(int sig) {
    if (sig != SIGUSR1) { exit_flag++; }
    if (exit_flag > 10) { abort(); }
}

void *signal_thread(void *arg) {
    int sig = 0;
    struct sigaction act = {.sa_handler = signal_handler,
                            .sa_flags = SA_INTERRUPT,
                            .sa_mask = *(sigset_t *)arg};

    sigaction(SIGUSR1, &act, NULL);
    if (sigwait(&act.sa_mask, &sig)) {
        exit_flag++;
        log_errno(FATAL, NULL, "sigwait", errno);
        return (void *)EXIT_FAILURE;
    }
    logi(NULL, "signal_thread", "Signal %d received", sig);
    close_threads();
    return (void *)EXIT_SUCCESS;
}
