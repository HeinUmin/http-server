#ifndef LOG_H
#define LOG_H

#include <arpa/inet.h>
#define NR_LOG_LEVEL 6

extern const char *LEVEL_STRING[];
typedef enum { TRACE, DEBUG, INFO, WARN, ERROR, FATAL } LogLevel;

int init_log(LogLevel level);
int close_log(void);
int error_log(LogLevel level, const char *src, const char *msg);
int access_log(int code, const char *request, size_t sent);
void log_errno(LogLevel level, const char *src, int errnum);
void log_format(int level, const char *src, const char *format, ...)
    __attribute__((format(printf, 3, 4)));

#define logt(...) log_format(TRACE, __VA_ARGS__)
#define logd(...) log_format(DEBUG, __VA_ARGS__)
#define logi(...) log_format(INFO, __VA_ARGS__)
#define logw(...) log_format(WARN, __VA_ARGS__)
#define loge(...) log_format(ERROR, __VA_ARGS__)
#define logf(...) log_format(FATAL, __VA_ARGS__)

#endif
