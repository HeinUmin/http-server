#ifndef PARSER_H
#define PARSER_H

#include <complex.h>
#define HTTP_BUF_SIZE 8192
#define HTTP_URI_SIZE 4096
#define NR_HTTP_METHOD 2

typedef enum { GET, HEAD } HttpMethod;
typedef enum {
    OK,
    MOVED,
    BAD_REQUEST,
    FORBIDDEN,
    NOT_FOUND,
    URI_LONG,
    SERVER_ERROR,
    NOT_IMPLEMENTED,
    NOT_SUPPORTED
} Response;

typedef struct {
    char http_uri[HTTP_URI_SIZE];
    HttpMethod http_method;
    int http_version;
    int connection;
} Request;

extern const char *HTTP_METHOD_STRING[];
extern const char *RESPONSE_STRING[];
extern const int RESPONSE_CODE[];

Response parse_request(char *buf, Request *request);

#endif
