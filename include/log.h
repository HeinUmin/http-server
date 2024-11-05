#ifndef LOG_H
#define LOG_H

#include <arpa/inet.h>

/**
 * @brief Log level strings
 *
 */
extern const char *LEVEL_STRING[];
extern const int MAX_LOG_LEVEL;

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
int error_log(long level, struct sockaddr_in addr, char *src, char *msg);

/**
 * @brief Log an access message
 *
 * @param addr client address
 * @param request request string
 * @param code response code
 * @param sent bytes sent
 * @return int 0 on success
 */
int access_log(struct sockaddr_in addr, char *request, int code, ssize_t sent);

/**
 * @brief Flush the error log
 *
 * @return int 0 on success
 */
int flush_error_log(void);

/**
 * @brief Flush the access log
 *
 * @return int 0 on success
 */
int flush_access_log(void);

#endif
