#ifndef __pthread_mutex_t_defined
#define __pthread_mutex_t_defined 1

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <bits/pthread_mutexattr_t.h>

typedef struct {
    int __lock;
    int __count_if_recursive;
    pthread_mutexattr_t __attr;
} pthread_mutex_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __pthread_mutex_t_defined */