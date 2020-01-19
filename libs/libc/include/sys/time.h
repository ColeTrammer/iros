#ifndef _SYS_TIME_H
#define _SYS_TIME_H 1

#include <bits/timeval.h>
#include <sys/select.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
};

int gettimeofday(struct timeval *__restrict tv, void *__restrict tz);
int utimes(const char *path, const struct timeval times[2]);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SYS_TIME_H */