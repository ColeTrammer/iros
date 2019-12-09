#include <errno.h>
#include <pthread.h>
#include <stddef.h>

int pthread_mutexattr_init(pthread_mutexattr_t *mutexattr) {
    if (mutexattr == NULL) {
        return EINVAL;
    }

    mutexattr->__flags = PTHREAD_PROCESS_PRIVATE;
    return 0;
}

int pthread_mutexattr_destroy(pthread_mutexattr_t *mutexattr) {
    if (mutexattr == NULL || mutexattr->__flags == -1) {
        return EINVAL;
    }

    mutexattr->__flags = -1;
    return 0;
}

int pthread_mutexattr_getpshared(pthread_mutexattr_t *__restrict mutexattr, int *__restrict pshared) {
    if (mutexattr == NULL || mutexattr->__flags == -1 || pshared == NULL) {
        return EINVAL;
    }

    *pshared = mutexattr->__flags & PTHREAD_PROCESS_SHARED;
    return 0;
}

int pthread_mutexattr_setpshared(pthread_mutexattr_t *mutexattr, int pshared) {
    if (mutexattr == NULL || (pshared != PTHREAD_PROCESS_PRIVATE && pshared != PTHREAD_PROCESS_SHARED)) {
        return EINVAL;
    }

    mutexattr->__flags |= pshared;
    return 0;
}