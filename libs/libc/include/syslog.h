#ifndef _SYSLOG_H
#define _SYSLOG_H 1

#define LOG_PID    1
#define LOG_CONS   2
#define LOG_NDELAY 4
#define LOG_ODELAY 8
#define LOG_NOWAIT 16

#define LOG_KERN   1
#define LOG_USER   0
#define LOG_MAIL   2
#define LOG_NEWS   3
#define LOG_UUCP   4
#define LOG_DAEMON 5
#define LOG_AUTH   6
#define LOG_CRON   7
#define LOG_LPR    8
#define LOG_LOCAL0 9
#define LOG_LOCAL1 10
#define LOG_LOCAL2 11
#define LOG_LOCAL3 12
#define LOG_LOCAL4 13
#define LOG_LOCAL5 14
#define LOG_LOCAL6 15
#define LOG_LOCAL7 16

#define LOG_EMERG   17
#define LOG_ALERT   18
#define LOG_CRIT    19
#define LOG_ERR     20
#define LOG_WARNING 21
#define LOG_NOTICE  22
#define LOG_INFO    23
#define LOG_DEBUG   24

#define LOG_MASK(pri) ((pri) & (LOG_EMERG | LOG_ALERT | LOG_CRIT | LOG_ERR | LOG_WARNING | LOG_NOTICE | LOG_INFO | LOG_DEBUG))

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void closelog(void);
void openlog(const char *ident, int option, int facility);
void setlogmask(int mask);
void syslog(int prio, const char *format, ...);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SYSLOG_H */