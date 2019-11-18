#ifndef _SYS_TIME_H
#define _SYS_TIME_H 1

#include <bits/suseconds_t.h>
#include <bits/time_t.h>
#include <sys/select.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct timeval {
    time_t tv_sec;
    suseconds_t tv_usec;
};

#ifndef __is_libk

time_t get_time();

#endif /* __is_libk */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SYS_TIME_H */