#ifndef _TIME_H
#define _TIME_H 1

#include <bits/clock_t.h>
#include <bits/clockid_t.h>
#include <bits/locale_t.h>
#include <bits/null.h>
#include <bits/pid_t.h>
#include <bits/size_t.h>
#include <bits/time_t.h>
#include <bits/timer_t.h>

#define CLOCKS_PER_SEC ((clock_t) 1000000)

#define CLOCK_MONOTONIC          0
#define CLOCK_PROCESS_CPUTIME_ID 1
#define CLOCK_REALTIME           2
#define CLOCK_THREAD_CPUTIME_ID  3

#define TIMER_ABSTIME 1

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct sigevent;

extern char *tzname[2];
extern long timezone;
extern int daylight;

struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

struct timespec {
    time_t tv_sec;
    long tv_nsec;
};

struct itimerspec {
    struct timespec it_interval;
    struct timespec it_value;
};

time_t time(time_t *tloc);
clock_t clock(void);
double difftime(time_t t1, time_t t2);
time_t mktime(struct tm *tm);

char *asctime(const struct tm *tm);
char *asctime_r(const struct tm *__restrict timeptr, char *__restrict result);
char *ctime(const time_t *t);
char *ctime_r(const time_t *__restrict t, char *__restrict result);

struct tm *gmtime(const time_t *t);
struct tm *gmtime_r(const time_t *__restrict timer, struct tm *__restrict result);
struct tm *localtime(const time_t *t);
struct tm *localtime_r(const time_t *__restrict timer, struct tm *__restrict result);

size_t strftime(char *__restrict s, size_t n, const char *__restrict format, const struct tm *__restrict tm);
char *strptime(const char *__restrict s, const char *__restrict format, struct tm *__restrict result);

int nanosleep(const struct timespec *amount, struct timespec *remaining);

int clock_getcpuclockid(pid_t pid, clockid_t *id);
int clock_getres(clockid_t id, struct timespec *res);
int clock_gettime(clockid_t id, struct timespec *res);
int clock_nanosleep(clockid_t id, int flags, const struct timespec *amount, struct timespec *remaining);
int clock_settime(clockid_t id, const struct timespec *to);

int timer_create(clockid_t id, struct sigevent *__restrict sig, timer_t *__restrict timer);
int timer_delete(timer_t timer);
int timer_getoverrun(timer_t timer);
int timer_gettime(timer_t timer, struct itimerspec *time);
int timer_settime(timer_t timer, int flags, const struct itimerspec *__restrict new_time, struct itimerspec *__restrict old);

void tzset(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _TIME_H */
