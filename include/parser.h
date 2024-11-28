#ifndef PARSER_H
#define PARSER_H

#define HTTP_BUF_SIZE 8192
#define HTTP_URI_SIZE 4096
#define NR_HTTP_METHOD 2

typedef enum { GET, HEAD } HttpMethod;
typedef enum {
    OK,
    PARTIAL,
    MOVED,
    BAD_REQUEST,
    FORBIDDEN,
    NOT_FOUND,
    URI_LONG,
    RANGE_UNSAT,
    SERVER_ERROR,
    NOT_IMPLEMENTED,
    NOT_SUPPORTED
} Response;

typedef struct {
    char http_host[HTTP_URI_SIZE];
    char http_uri[HTTP_URI_SIZE];
    HttpMethod http_method;
    int http_version;
    int connection;
    long range_start;
    long range_end;
} Request;

typedef struct {
    char type[32];
    char time[32];
    char path[HTTP_URI_SIZE];
    long size;
} FileInfo;

extern const char *HTTP_METHOD_STRING[];
extern const char *RESPONSE_STRING[];
extern const int RESPONSE_CODE[];

Response parse_request(char *buf, Request *request, int https);
Response parse_uri(const Request *request, FileInfo *fileinfo);

#endif
