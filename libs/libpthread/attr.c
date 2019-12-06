#define __libc_internal

#include <errno.h>
#include <pthread.h>

int pthread_attr_destroy(pthread_attr_t *attr) {
    *attr = -1;
    return 0;
}

int pthread_attr_init(pthread_attr_t *attr) {
    *attr = PTHREAD_CREATE_JOINABLE;
    return 0;
}

int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate) {
    if (*attr == -1) {
        return EINVAL;
    }

    *detachstate = *attr & PTHREAD_CREATE_DETACHED;
    return 0;
}
int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate) {
    if (detachstate != PTHREAD_CREATE_DETACHED && detachstate != PTHREAD_CREATE_JOINABLE) {
        return EINVAL;
    }

    *attr |= detachstate & PTHREAD_CREATE_DETACHED;
    return 0;
}