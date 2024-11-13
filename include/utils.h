#ifndef UTILS_H
#define UTILS_H

#include <arpa/inet.h>

#define TIMESTRLEN 32
extern __thread struct sockaddr_in sock;

size_t get_time(char *time_str, const char *format);

#endif
