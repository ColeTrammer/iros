#ifndef __pthread_cond_t_defined
#define __pthread_cond_t_defined 1

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <bits/pthread_condattr_t.h>

typedef struct {
    unsigned int __lock;
    pthread_condattr_t __attr;
} pthread_cond_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __pthread_cond_t_defined */