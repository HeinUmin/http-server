#include "parser.h"
#include <stdlib.h>
#include <string.h>

const char *HTTP_METHOD_STRING[] = {"GET", "HEAD"};
const char *RESPONSE_STRING[] = {"OK",
                                 "Moved Permanently",
                                 "Bad Request",
                                 "Forbidden",
                                 "Not Found",
                                 "URI Too Long",
                                 "Internal Server Error",
                                 "Not Implemented",
                                 "HTTP Version Not Supported"};
const int RESPONSE_CODE[] = {200, 301, 400, 403, 404, 414, 500, 501, 505};

Response parse_request(char *buf, Request *request) {
    int temp = 0;
    char *line = NULL, *token = NULL;
    char *lineptr = NULL, *tokptr = NULL;

    // Parse request line
    line = strtok_r(buf, "\r\n", &lineptr);
    if (!line) { return BAD_REQUEST; }
    // Parse request method
    token = strtok_r(line, " ", &tokptr);
    if (!token) { return BAD_REQUEST; }
    for (temp = 0; temp < NR_HTTP_METHOD; temp++) {
        if (strcmp(token, HTTP_METHOD_STRING[temp]) == 0) {
            request->http_method = temp;
            break;
        }
    }
    if (temp == NR_HTTP_METHOD) { return NOT_IMPLEMENTED; }
    // Parse request URI
    token = strtok_r(NULL, " ", &tokptr);
    if (!token) { return BAD_REQUEST; }
    if (strlen(token) >= HTTP_URI_SIZE) { return URI_LONG; }
    strlcpy(request->http_uri, token, HTTP_URI_SIZE);
    // Parse request version
    token = strtok_r(NULL, " ", &tokptr);
    if (!token) { return BAD_REQUEST; }
    if (strcmp(token, "HTTP/1.0") == 0) {
        request->http_version = 0;
        request->connection = 0;
    } else if (strcmp(token, "HTTP/1.1") == 0) {
        request->http_version = 1;
        request->connection = 1;
    } else {
        return NOT_SUPPORTED;
    }
    // Check request line
    if ((token = strtok_r(NULL, " ", &tokptr))) { return BAD_REQUEST; }

    // Parse request headers
    while ((line = strtok_r(NULL, "\r\n", &lineptr))) {
        token = strtok_r(line, ":", &tokptr);
        if (!token) { return BAD_REQUEST; }
        while (tokptr || tokptr[0] == ' ' || tokptr[0] == '\t') { tokptr++; }
        if (!tokptr) { return BAD_REQUEST; }
        if (strcasecmp(token, "Connection") != 0) { continue; }
        if (strcasecmp(tokptr, "close") == 0) {
            request->connection = 0;
        } else if (strcasecmp(tokptr, "keep-alive") == 0) {
            request->connection = 1;
        }
    }
    return 0;
}

#undef return_error
