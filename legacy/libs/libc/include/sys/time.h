#ifndef _SYS_TIME_H
#define _SYS_TIME_H 1

#include <bits/timeval.h>
#include <sys/select.h>

#define ITIMER_REAL    1
#define ITIMER_VIRTUAL 2
#define ITIMER_PROF    3

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct itimerval {
    struct timeval it_interval;
    struct timeval it_value;
};

struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
};

int getitimer(int which, struct itimerval *valp);
int setitimer(int which, const struct itimerval *nvalp, struct itimerval *ovalp);
int gettimeofday(struct timeval *__restrict tv, void *__restrict tz);
int utimes(const char *path, const struct timeval times[2]);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SYS_TIME_H */
