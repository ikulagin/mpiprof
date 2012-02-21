/*
 * log.c
 *
 *  Created on: 21.02.2012
 *      Author: ikulagin
 */
#include "log.h"


void apptrace(int level, char *format, ...)
{
    if (!(log_opt.tracemask & level)) {
        return;
    }

    va_list ap;
    int length, size, loglevel;
    char str[4096], timestamp[100], *p;
    struct timeval tp;
    struct tm *pt;
    time_t t;

    gettimeofday(&tp, NULL);
    t = (time_t) tp.tv_sec;
    pt = localtime(&t);
    sprintf(timestamp, "% 2d:%02d:%02d.%03d", pt->tm_hour, pt->tm_min,
            pt->tm_sec, (int) tp.tv_usec / 1000);
    loglevel = LOG_UNUSE;
    switch (level) {
    case TRACE_INFO_BIT:
        p = "[INFO]";
        loglevel = LOG_INFO;
        break;
    case TRACE_ERR_BIT:
        p = "[ERR ]";
        loglevel = LOG_ERR;
        break;
    case TRACE_WARN_BIT:
        p = "[WARN]";
        loglevel = LOG_WARNING;
        break;
    case TRACE_DBG_BIT:
        p = "[DBG]";
        loglevel = LOG_DEBUG;
        break;
    default:
        p = "[????]";
        break;
    }
    sprintf(str, "%s  %s  ", timestamp, p);
    length = strlen(str);
    size = sizeof(str) - length - 10;

    va_start(ap, format);
    length = vsnprintf(&str[length], size, format, ap);
    va_end(ap);

    size = strlen(str);
    while (size && (str[size - 1] == '\n'))
        size--;

    if (log_opt.output == DEVCONSOLE)
        printf("%s", str);
    else
        fprintf(stderr, "%s\n", str);
}


