#include "parser.h"
#include "utils.h"

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

const char *HTTP_METHOD_STRING[] = {"GET", "HEAD"};
const char *RESPONSE_STRING[] = {"OK",
                                 "Partial Content",
                                 "Moved Permanently",
                                 "Bad Request",
                                 "Forbidden",
                                 "Not Found",
                                 "URI Too Long",
                                 "Range Not Satisfiable",
                                 "Internal Server Error",
                                 "Not Implemented",
                                 "HTTP Version Not Supported"};
const int RESPONSE_CODE[] = {200, 206, 301, 400, 403, 404,
                             414, 416, 500, 501, 505};

static size_t url_decode(char *str) {
    char hex[3] = {0};
    char *dest = str;
    char *data = str;
    size_t len = strlen(str);

    while (len--) {
        switch (*data) {
        case '+':
            *dest = ' ';
            break;
        case '%':
            if (len >= 2 && isxdigit(*(data + 1)) && isxdigit(*(data + 2))) {
                hex[0] = *(data + 1), hex[1] = *(data + 2);
                *dest = (char)strtol(hex, NULL, 16);
                data += 2, len -= 2;
            } else {
                *dest = *data;
            }
            break;
        case '?':
            *dest = '\0';
            return dest - str;
        default:
            *dest = *data;
        }
        data++;
        dest++;
    }
    *dest = '\0';
    return dest - str;
}

static inline void get_mime(char *filename, char *filetype) {
    if (strstr(filename, ".html")) {
        strcpy(filetype, "text/html");
    } else if (strstr(filename, ".gif")) {
        strcpy(filetype, "image/gif");
    } else if (strstr(filename, ".png")) {
        strcpy(filetype, "image/png");
    } else if (strstr(filename, ".css")) {
        strcpy(filetype, "text/css");
    } else if (strstr(filename, ".jpg") || strstr(filename, ".jpeg")) {
        strcpy(filetype, "image/jpeg");
    } else if (strstr(filename, ".mp4")) {
        strcpy(filetype, "video/mp4");
    } else {
        strcpy(filetype, "application/octet-stream");
    }
}

Response parse_request(char *buf, Request *request, int https) {
    int temp = 0;
    char request_line[HTTP_URI_SIZE + 32] = {0};  // To avoid buffer overflow
    char *line = NULL, *token = NULL, *tmpptr = NULL;
    char *lineptr = NULL, *tokptr = NULL;
    if (!buf || !request) { return SERVER_ERROR; }
    *request = (Request){0};

    // Parse request line
    line = strtok_r(buf, "\r\n", &lineptr);
    if (!line) { return BAD_REQUEST; }
    if (strlen(line) >= HTTP_URI_SIZE + 32) { return BAD_REQUEST; }
    strncpy(request_line, line, HTTP_URI_SIZE + 32);
    // Parse request method
    token = strtok_r(request_line, " ", &tokptr);
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
    if (token[0] != '/') {
        // Parse http host
        if (strncasecmp(token, "http://", 7) == 0) {
            token += 7;
        } else if (strncasecmp(token, "https://", 8) == 0) {
            token += 8;
        }
        tmpptr = strchr(token, '/');
        if (!tmpptr) { return BAD_REQUEST; }
        if (tmpptr - token >= HTTP_URI_SIZE) { return URI_LONG; }
        strncpy(request->http_host, token, tmpptr - token);
        token = tmpptr;
    }
    if (strlen(token) >= HTTP_URI_SIZE) { return URI_LONG; }
    strncpy(request->http_uri, token, HTTP_URI_SIZE);
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
    if (strtok_r(NULL, " ", &tokptr)) { return BAD_REQUEST; }

    // Parse request headers
    temp = 0;
    while ((line = strtok_r(NULL, "\r\n", &lineptr))) {
        token = strtok_r(line, ":", &tokptr);
        if (!token) { return BAD_REQUEST; }
        if (strcasecmp(token, "Connection") == 0) {
            // Parse Connection header
            token = strtok_r(NULL, ", \t", &tokptr);
            if (!token) { continue; }
            if (strcasecmp(token, "close") == 0) {
                request->connection = 0;
            } else if (strcasecmp(token, "keep-alive") == 0) {
                request->connection = 1;
            }
        } else if (strcasecmp(token, "Host") == 0) {
            // Parse Host header
            if (temp++) { return BAD_REQUEST; }
            if (request->http_host[0] != '\0') { continue; }
            token = strtok_r(NULL, " \t", &tokptr);
            if (!token) { continue; }
            if (strncasecmp(token, "http://", 7) == 0) {
                token += 7;
            } else if (strncasecmp(token, "https://", 8) == 0) {
                token += 8;
            }
            if (strlen(token) >= HTTP_URI_SIZE) { return URI_LONG; }
            strncpy(request->http_host, token, HTTP_URI_SIZE);
            if (strtok_r(NULL, " \t", &tokptr)) { return BAD_REQUEST; }
        } else if (strcasecmp(token, "Range") == 0) {
            token = strtok_r(NULL, "= \t", &tokptr);
            if (!token) { continue; }
            if (strcasecmp(token, "bytes") != 0) { return RANGE_UNSAT; }
            token = strtok_r(NULL, "-", &tokptr);
            if (!token) { return RANGE_UNSAT; }
            request->range_start = strtol(token, NULL, 10);
            if (request->range_start < 0) { return RANGE_UNSAT; }
            token = strtok_r(NULL, ", \t", &tokptr);
            if (!token) {
                request->range_end = LONG_MAX;
                continue;
            }
            request->range_end = strtol(token, NULL, 10);
            if (request->range_end < request->range_start) {
                return RANGE_UNSAT;
            }
        }
    }
    // Check request headers
    if (request->http_host[0] == '\0') {
        if (!request->http_version) {
            if (sprintf(request->http_host, "localhost") < 0) {
                return SERVER_ERROR;
            }
        } else {
            return BAD_REQUEST;
        }
    }
    strtok_r(request->http_host, ":", &tmpptr);
    return (https || !port[1 - https]) ? OK : MOVED;
}

Response parse_uri(const Request *request, FileInfo *fileinfo) {
    struct stat sbuf = {0};
    struct tm timeinfo;
    if (!request || !fileinfo ||
        sprintf(fileinfo->path, ".%s", request->http_uri) < 0) {
        return SERVER_ERROR;
    }
    url_decode(fileinfo->path);
    if (stat(fileinfo->path, &sbuf) < 0) { return NOT_FOUND; }
    if (S_ISDIR(sbuf.st_mode)) {
        strcat(fileinfo->path, "/index.html");
        if (stat(fileinfo->path, &sbuf) < 0) { return NOT_FOUND; }
    }
    if (!S_ISREG(sbuf.st_mode) || !(S_IRUSR & sbuf.st_mode)) {
        return FORBIDDEN;
    }
    if (!strftime(fileinfo->time, sizeof(fileinfo->time),
                  "%a, %d %b %Y %H:%M:%S GMT",
                  gmtime_r(&sbuf.st_mtime, &timeinfo))) {
        return SERVER_ERROR;
    }
    get_mime(fileinfo->path, fileinfo->type);
    fileinfo->size = sbuf.st_size;
    return OK;
}
