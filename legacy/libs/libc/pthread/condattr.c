#include <errno.h>
#include <pthread.h>
#include <stddef.h>

int pthread_condattr_init(pthread_condattr_t *condattr) {
    if (condattr == NULL) {
        return EINVAL;
    }

    condattr->__clockid = CLOCK_MONOTONIC;
    condattr->__flags = 0;
    return 0;
}

int pthread_condattr_destroy(pthread_condattr_t *condattr) {
    if (condattr == NULL || condattr->__flags == -1) {
        return EINVAL;
    }

    condattr->__flags = -1;
    return 0;
}

int pthread_condattr_getclock(pthread_condattr_t *__restrict condattr, clockid_t *__restrict clock) {
    if (condattr == NULL || condattr->__flags == -1 || clock == NULL) {
        return EINVAL;
    }

    *clock = condattr->__clockid;
    return 0;
}

int pthread_condattr_getpshared(pthread_condattr_t *__restrict condattr, int *__restrict pshared) {
    if (condattr == NULL || condattr->__flags == -1 || pshared == NULL) {
        return EINVAL;
    }

    *pshared = condattr->__flags & PTHREAD_PROCESS_SHARED;
    return 0;
}

int pthread_condattr_setclock(pthread_condattr_t *condattr, clockid_t clock) {
    if (condattr == NULL || condattr->__flags == -1 || (clock != CLOCK_MONOTONIC && clock != CLOCK_REALTIME)) {
        return EINVAL;
    }

    condattr->__clockid = clock;
    return 0;
}

int pthread_condattr_setpshared(pthread_condattr_t *__restrict condattr, int pshared) {
    if (condattr == NULL || condattr->__flags == -1 || (pshared != PTHREAD_PROCESS_PRIVATE && pshared != PTHREAD_PROCESS_SHARED)) {
        return EINVAL;
    }

    condattr->__flags |= pshared;
    return 0;
}
