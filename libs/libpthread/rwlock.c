#include <pthread.h>

int pthread_rwlock_destroy(pthread_rwlock_t *rwlock) {
    (void) rwlock;
    return 0;
}

int pthread_rwlock_init(pthread_rwlock_t *__restrict rwlock, const pthread_rwlockattr_t *__restrict rwlockattr) {
    (void) rwlock;
    (void) rwlockattr;
    return 0;
}

int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock) {
    (void) rwlock;
    return 0;
}

int pthread_rwlock_timedrdlock(pthread_rwlock_t *__restrict rwlock, const struct timespec *__restrict timeout) {
    (void) rwlock;
    (void) timeout;
    return 0;
}

int pthread_rwlock_timedwrlock(pthread_rwlock_t *__restrict rwlock, const struct timespec *__restrict timeout) {
    (void) rwlock;
    (void) timeout;
    return 0;
}

int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock) {
    (void) rwlock;
    return 0;
}

int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock) {
    (void) rwlock;
    return 0;
}

int pthread_rwlock_unlock(pthread_rwlock_t *rwlock) {
    (void) rwlock;
    return 0;
}

int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock) {
    (void) rwlock;
    return 0;
}
