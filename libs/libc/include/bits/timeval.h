#ifndef __timeval_defined
#define __timeval_defined 1

#include <bits/suseconds_t.h>
#include <bits/time_t.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct timeval {
    time_t tv_sec;
    suseconds_t tv_usec;
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __timeval_defined */
