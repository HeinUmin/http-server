#include "utils.h"
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
