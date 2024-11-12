#ifndef UTILS_H
#define UTILS_H

#include <arpa/inet.h>

#define TIMESTRLEN 32
#define HTTP_BUF_SIZE 8192
#define HTTP_HEADER_NAME_SIZE 4096
#define HTTP_HEADER_VALUE_SIZE 4096
#define HTTP_VERSION_SIZE 16
#define HTTP_METHOD_SIZE 16
#define HTTP_URI_SIZE 4096

typedef struct {
    char name[HTTP_HEADER_NAME_SIZE];
    char value[HTTP_HEADER_VALUE_SIZE];
} RequestHeader;

typedef struct {
    char http_version[HTTP_VERSION_SIZE];
    char http_method[HTTP_METHOD_SIZE];
    char http_uri[HTTP_URI_SIZE];
    int header_count;
    RequestHeader *headers;
} Request;

extern __thread struct sockaddr_in sock;

size_t get_time(char *time_str, const char *format);
Request *parse_request(char *buf);

#endif
