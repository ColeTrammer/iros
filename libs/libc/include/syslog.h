#ifndef _SYSLOG_H
#define _SYSLOG_H 1

#include <stdarg.h>

#define LOG_PID    1
#define LOG_CONS   2
#define LOG_NDELAY 4
#define LOG_ODELAY 8
#define LOG_NOWAIT 16
#define LOG_PERROR 32

#define LOG_KERN   (0 << 3)
#define LOG_USER   (1 << 3)
#define LOG_MAIL   (2 << 3)
#define LOG_NEWS   (3 << 3)
#define LOG_UUCP   (4 << 3)
#define LOG_DAEMON (5 << 3)
#define LOG_AUTH   (6 << 3)
#define LOG_CRON   (7 << 3)
#define LOG_LPR    (8 << 3)
#define LOG_LOCAL0 (9 << 3)
#define LOG_LOCAL1 (10 << 3)
#define LOG_LOCAL2 (11 << 3)
#define LOG_LOCAL3 (12 << 3)
#define LOG_LOCAL4 (13 << 3)
#define LOG_LOCAL5 (14 << 3)
#define LOG_LOCAL6 (15 << 3)
#define LOG_LOCAL7 (16 << 3)

#define LOG_EMERG   0
#define LOG_ALERT   1
#define LOG_CRIT    2
#define LOG_ERR     3
#define LOG_WARNING 4
#define LOG_NOTICE  5
#define LOG_INFO    6
#define LOG_DEBUG   7

#define LOG_MASK(pri) (1 << (pri))

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void closelog(void);
void openlog(const char *ident, int option, int facility);
int setlogmask(int mask);
void syslog(int prio, const char *format, ...);
void vsyslog(int prio, const char *format, va_list args);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SYSLOG_H */
