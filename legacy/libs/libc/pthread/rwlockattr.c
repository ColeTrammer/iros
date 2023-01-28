#include <errno.h>
#include <pthread.h>

int pthread_rwlockattr_destroy(pthread_rwlockattr_t *rwlockattr) {
    if (rwlockattr == NULL || rwlockattr->__flags == -1) {
        return EINVAL;
    }

    rwlockattr->__flags = -1;
    return 0;
}

int pthread_rwlockattr_init(pthread_rwlockattr_t *rwlockattr) {
    if (rwlockattr == NULL) {
        return EINVAL;
    }

    rwlockattr->__flags = 0;
    return 0;
}

int pthread_rwlockattr_getpshared(const pthread_rwlockattr_t *__restrict rwlockattr, int *__restrict pshared) {
    if (rwlockattr == NULL || rwlockattr->__flags == -1 || pshared == NULL) {
        return EINVAL;
    }

    *pshared = rwlockattr->__flags & PTHREAD_PROCESS_SHARED;
    return 0;
}

int pthread_rwlockattr_setpshared(pthread_rwlockattr_t *rwlockattr, int pshared) {
    if (rwlockattr == NULL || rwlockattr->__flags == -1 || (pshared != PTHREAD_PROCESS_PRIVATE && pshared != PTHREAD_PROCESS_SHARED)) {
        return EINVAL;
    }

    rwlockattr->__flags |= pshared;
    return 0;
}
