#include <errno.h>
#include <pthread.h>

int pthread_barrierattr_destroy(pthread_barrierattr_t *barrierattr) {
    if (barrierattr == NULL || barrierattr->__flags == -1) {
        return EINVAL;
    }

    barrierattr->__flags = -1;
    return 0;
}

int pthread_barrierattr_init(pthread_barrierattr_t *barrierattr) {
    if (barrierattr == NULL) {
        return EINVAL;
    }

    barrierattr->__flags = 0;
    return 0;
}

int pthread_barrierattr_getpshared(const pthread_barrierattr_t *__restrict barrierattr, int *__restrict pshared) {
    if (barrierattr == NULL || barrierattr->__flags == -1 || pshared == NULL) {
        return EINVAL;
    }

    *pshared = barrierattr->__flags & PTHREAD_PROCESS_SHARED;
    return 0;
}

int pthread_barrierattr_setpshared(pthread_barrierattr_t *barrierattr, int pshared) {
    if (barrierattr == NULL || barrierattr->__flags == -1 || (pshared != PTHREAD_PROCESS_PRIVATE && pshared != PTHREAD_PROCESS_SHARED)) {
        return EINVAL;
    }

    barrierattr->__flags |= pshared;
    return 0;
}
