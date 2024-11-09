#include "sighandler.h"
#include "log.h"

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ERR_BUF_SIZE 64

volatile int exit_flag = 0;
struct Thread_poll *thread_poll = NULL;

void close_threads(void) {
    int ret = 0;
    char err_buf[ERR_BUF_SIZE];
    struct Thread_poll *cur_thread = NULL;
    exit_flag++;
    while (thread_poll) {
        cur_thread = thread_poll;
        ret = pthread_kill(cur_thread->thread, SIGUSR1);
        if (ret) {
            strerror_r(ret, err_buf, ERR_BUF_SIZE);
            error_log(WARN, NULL, "pthread_kill", err_buf);
        }
        thread_poll = thread_poll->next;
        free(cur_thread);
    }
}

void insert_thread(pthread_t thread) {
    char err_buf[ERR_BUF_SIZE];
    struct Thread_poll *new_thread = malloc(sizeof(struct Thread_poll));
    if (!new_thread) {
        strerror_r(errno, err_buf, ERR_BUF_SIZE);
        error_log(FATAL, NULL, "malloc", err_buf);
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
    char err_buf[ERR_BUF_SIZE];
    struct sigaction act = {.sa_handler = signal_handler,
                            .sa_flags = SA_INTERRUPT,
                            .sa_mask = *(sigset_t *)arg};

    if (sigaction(SIGUSR1, &act, NULL)) {
        strerror_r(errno, err_buf, ERR_BUF_SIZE);
        error_log(FATAL, NULL, "sigaction", err_buf);
        exit_flag++;
        return (void *)EXIT_FAILURE;
    }

    if (sigwait(&act.sa_mask, &sig)) {
        strerror_r(errno, err_buf, ERR_BUF_SIZE);
        error_log(FATAL, NULL, "sigwait", err_buf);
        exit_flag++;
        return (void *)EXIT_FAILURE;
    }
    if (sprintf(err_buf, "Signal %d received", sig) < 0) {
        strerror_r(errno, err_buf, ERR_BUF_SIZE);
        error_log(WARN, NULL, "snprintf", err_buf);
    } else {
        error_log(INFO, NULL, "signal_thread", err_buf);
    }

    close_threads();
    return (void *)EXIT_SUCCESS;
}
