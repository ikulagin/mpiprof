/*
 * log.h
 *
 *  Created on: 21.02.2012
 *      Author: ikulagin
 */

#ifndef LOG_H_
#define LOG_H_

#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>

#define LOG_UNUSE       0       /* system is unusable */
#define LOG_ERR         1       /* error conditions */
#define LOG_WARNING     2       /* warning conditions */
#define LOG_INFO        3       /* informational */
#define LOG_DEBUG       4       /* debug-level messages */

#define TRACEMASKDEF (0x01 | 0x02 | 0x04 | 0x08)
#define TRACE_ERR_BIT 0x01
#define TRACE_WARN_BIT 0x02
#define TRACE_INFO_BIT 0x04
#define TRACE_DBG_BIT 0x08

#define DEVCONSOLE 0
#define STDERR 1

struct traceopt {
    int tracemask;
    int output;     // 0 - dev console; 1 - srderr;
};

struct traceopt log_opt;



void apptrace(int level, char *format, ...);


#endif /* LOG_H_ */
