#ifndef __pthread_barrier_t_defined
#define __pthread_barrier_t_defined 1

#include <bits/pthread_barrierattr_t.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
    unsigned int __count_now;
    unsigned int __count_to;
    unsigned int __count_gap;
    pthread_barrierattr_t __attr;
} pthread_barrier_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __pthread_barrier_t_defined */
