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
int error_log(long level, const struct sockaddr_in *addr, char *src, char *msg);

/**
 * @brief Log an access message
 *
 * @param addr client address
 * @param request request string
 * @param code response code
 * @param sent bytes sent
 * @return int 0 on success
 */
int access_log(int code,
               const struct sockaddr_in *addr,
               char *request,
               ssize_t sent);

#endif
