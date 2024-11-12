#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

__thread struct sockaddr_in sock = {0};

size_t get_time(char *time_str, const char *format) {
    size_t ret = 0;
    struct tm timeinfo;
    time_t now = time(NULL);
    ret = strftime(time_str, TIMESTRLEN, format, localtime_r(&now, &timeinfo));
    if (!ret) { time_str[0] = '\0'; }
    return ret;
}

void free_request(Request *request) {
    if (request) {
        if (request->headers) { free(request->headers); }
        free(request);
    }
}

#define return_error()                                                         \
    do {                                                                       \
        free_request(request);                                                 \
        return NULL;                                                           \
    } while (0)

Request *parse_request(char *buf) {
    Request *request = (Request *)malloc(sizeof(Request));
    RequestHeader *headers = NULL;
    char *line = NULL, *token = NULL;
    char *lineptr = NULL, *tokptr = NULL;

    // Parse request line
    line = strtok_r(buf, "\r\n", &lineptr);
    if (!line) { return_error(); }
    token = strtok_r(line, " ", &tokptr);
    if (!token || strlen(token) >= HTTP_METHOD_SIZE) { return_error(); }
    strlcpy(request->http_method, token, HTTP_METHOD_SIZE);
    token = strtok_r(NULL, " ", &tokptr);
    if (!token || strlen(token) >= HTTP_URI_SIZE) { return_error(); }
    strlcpy(request->http_uri, token, HTTP_URI_SIZE);
    token = strtok_r(NULL, " ", &tokptr);
    if (!token || strlen(token) >= HTTP_VERSION_SIZE) { return_error(); }
    strlcpy(request->http_version, token, HTTP_VERSION_SIZE);
    token = strtok_r(NULL, " ", &tokptr);
    if (token) { return_error(); }

    // Initialize request headers
    request->header_count = 0;
    headers = (RequestHeader *)malloc(0);
    if (!headers) { return_error(); }

    // Parse request headers
    while ((line = strtok_r(NULL, "\r\n", &lineptr))) {
        headers = realloc(request->headers,
                          ++request->header_count * sizeof(RequestHeader));
        token = strtok_r(line, ":", &tokptr);
        if (!token || strlen(token) >= HTTP_HEADER_NAME_SIZE) {
            return_error();
        }
        strlcpy(headers[request->header_count - 1].name, token,
                HTTP_HEADER_NAME_SIZE);
        while (tokptr || tokptr[0] == ' ' || tokptr[0] == '\t') { tokptr++; }
        if (!tokptr || strlen(tokptr) >= HTTP_HEADER_VALUE_SIZE) {
            return_error();
        }
        strlcpy(headers[request->header_count - 1].value, tokptr,
                HTTP_HEADER_VALUE_SIZE);
    }
    return 0;
}

#undef return_error
