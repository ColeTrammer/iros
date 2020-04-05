#ifndef __pthread_mutex_t_defined
#define __pthread_mutex_t_defined 1

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <bits/__locked_robust_mutex_node.h>
#include <bits/pthread_mutexattr_t.h>

typedef struct {
    unsigned int __lock;
    int __count_if_recursive;
    pthread_mutexattr_t __attr;
    struct __locked_robust_mutex_node __locked_robust_mutex_node;
} pthread_mutex_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __pthread_mutex_t_defined */