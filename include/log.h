#ifndef LOG_H
#define LOG_H

#include <arpa/inet.h>

#define NR_LOG_LEVEL 6
enum Log_level { TRACE, DEBUG, INFO, WARN, ERROR, FATAL };

/**
 * @brief Log level strings
 *
 */
extern const char *LEVEL_STRING[];

/**
 * @brief Initialize log files
 *
 * @param level Program log level
 * @return int 0 on success
 */
int init_log(long level);

/**
 * @brief Close log files
 *
 * @return int 0 on success
 */
int close_log(void);

/**
 * @brief Log an error message
 *
 * @param level message level
 * @param addr client address
 * @param src error source
 * @param msg error message
 * @return int 0 on success
 */
int error_log(long level,
              const struct sockaddr_in *addr,
              const char *src,
              const char *msg);

/**
 * @brief Log an access message
 *
 * @param code response code
 * @param addr client address
 * @param request request string
 * @param sent bytes sent
 * @return int 0 on success
 */
int access_log(int code,
               const struct sockaddr_in *addr,
               const char *request,
               ssize_t sent);

/**
 * @brief Log an error message with errno
 *
 * @param level message level
 * @param addr client address
 * @param src error source
 * @param errnum error number
 */
void log_errno(int level,
               const struct sockaddr_in *addr,
               const char *src,
               int errnum);

/**
 * @brief Log an error message with format
 *
 * @param level message level
 * @param addr client address
 * @param src error source
 * @param format error message format
 * @param ... error message arguments
 */
void log_format(int level,
                const struct sockaddr_in *addr,
                const char *src,
                const char *format,
                ...) __attribute__((format(printf, 4, 5)));

#define logt(...) log_format(TRACE, __VA_ARGS__)
#define logd(...) log_format(DEBUG, __VA_ARGS__)
#define logi(...) log_format(INFO, __VA_ARGS__)
#define logw(...) log_format(WARN, __VA_ARGS__)
#define loge(...) log_format(ERROR, __VA_ARGS__)
#define logf(...) log_format(FATAL, __VA_ARGS__)

#endif
