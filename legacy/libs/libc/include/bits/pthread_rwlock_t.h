#ifndef __pthread_rwlock_t_defined
#define __pthread_rwlock_t_defined 1

#include <bits/lock.h>
#include <bits/pthread_rwlockattr_t.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
    struct __lock __writer_lock;
    struct __lock __reader_lock;
    int __reader_count;
    int __reader_locked;
    pthread_rwlockattr_t __attr;
} pthread_rwlock_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __pthread_rwlock_t_defined */
