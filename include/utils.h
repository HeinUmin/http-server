#ifndef UTILS_H
#define UTILS_H

#include <arpa/inet.h>

#define TIMESTRLEN 32

extern __thread struct sockaddr_in sock;
extern uint16_t port[2];

size_t get_time(char *time_str, const char *format, int gmt);

#endif
