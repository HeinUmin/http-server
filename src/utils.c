#include "utils.h"
#include <time.h>

__thread struct sockaddr_in sock = {0};
uint16_t port[2] = {80, 443};

size_t get_time(char *time_str, const char *format, int gmt) {
    size_t ret = 0;
    struct tm timeinfo;
    time_t now = time(NULL);
    ret = strftime(time_str, TIMESTRLEN, format,
                   gmt ? gmtime_r(&now, &timeinfo) :
                         localtime_r(&now, &timeinfo));
    if (!ret) { time_str[0] = '\0'; }
    return ret;
}
