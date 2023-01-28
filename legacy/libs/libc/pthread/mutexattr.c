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

int pthread_mutexattr_getrobust(const pthread_mutexattr_t *__restrict mutexattr, int *__restrict robust) {
    if (mutexattr == NULL || mutexattr->__flags == -1 || robust == NULL) {
        return EINVAL;
    }

    *robust = mutexattr->__flags & PTHREAD_MUTEX_ROBUST;
    return 0;
}

int pthread_mutexattr_gettype(const pthread_mutexattr_t *__restrict mutexattr, int *__restrict type) {
    if (mutexattr == NULL || mutexattr->__flags == -1 || type == NULL) {
        return EINVAL;
    }

    *type = mutexattr->__flags & (PTHREAD_MUTEX_ERRORCHECK | PTHREAD_MUTEX_RECURSIVE);
    return 0;
}

int pthread_mutexattr_setpshared(pthread_mutexattr_t *mutexattr, int pshared) {
    if (mutexattr == NULL || (pshared != PTHREAD_PROCESS_PRIVATE && pshared != PTHREAD_PROCESS_SHARED)) {
        return EINVAL;
    }

    mutexattr->__flags |= pshared;
    return 0;
}

int pthread_mutexattr_settype(pthread_mutexattr_t *mutexattr, int type) {
    if (mutexattr == NULL || (type != PTHREAD_MUTEX_NORMAL && type != PTHREAD_MUTEX_ERRORCHECK && type != PTHREAD_MUTEX_RECURSIVE &&
                              type != PTHREAD_MUTEX_DEFAULT)) {
        return EINVAL;
    }

    mutexattr->__flags |= type;
    return 0;
}

int pthread_mutexattr_setrobust(pthread_mutexattr_t *mutexattr, int robust) {
    if (mutexattr == NULL || (robust != PTHREAD_MUTEX_STALLED && robust != PTHREAD_MUTEX_ROBUST)) {
        return EINVAL;
    }

    mutexattr->__flags |= robust;
    return 0;
}
