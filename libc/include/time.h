#ifndef _TIME_H
#define _TIME_H 1

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct timespec {
    time_t tv_sec;
    long tv_nsec;
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _TIME_H */