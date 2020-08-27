#include <errno.h>
#include <pthread.h>

int pthread_rwlock_destroy(pthread_rwlock_t *rwlock) {
    if (!rwlock || rwlock->__reader_count == -1) {
        return EINVAL;
    }

    rwlock->__reader_count = -1;
    return 0;
}

int pthread_rwlock_init(pthread_rwlock_t *__restrict rwlock, const pthread_rwlockattr_t *__restrict rwlockattr) {
    if (!rwlock) {
        return EINVAL;
    }

    if (rwlockattr) {
        rwlock->__attr = *rwlockattr;
    }

    rwlock->__writer_lock.__lock = rwlock->__reader_lock.__lock = rwlock->__reader_locked = rwlock->__reader_count = 0;
    return 0;
}

int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock) {
    __lock(&rwlock->__reader_lock);
    if (++rwlock->__reader_count == 1) {
        __lock(&rwlock->__writer_lock);
    }
    rwlock->__reader_locked = 1;
    __unlock(&rwlock->__reader_lock);
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
    if (__trylock(&rwlock->__reader_lock)) {
        return EBUSY;
    }

    if (++rwlock->__reader_count != 1) {
        goto success;
    }

    if (__trylock(&rwlock->__writer_lock)) {
        __unlock(&rwlock->__reader_lock);
        return EBUSY;
    }

success:
    rwlock->__reader_locked = 1;
    __unlock(&rwlock->__reader_lock);
    return 0;
}

int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock) {
    if (__trylock(&rwlock->__writer_lock)) {
        return EBUSY;
    }

    return 0;
}

int pthread_rwlock_unlock(pthread_rwlock_t *rwlock) {
    if (!rwlock->__reader_locked) {
        __unlock(&rwlock->__writer_lock);
        return 0;
    }

    __lock(&rwlock->__reader_lock);
    if (--rwlock->__reader_count == 0) {
        rwlock->__reader_locked = 0;
        __unlock(&rwlock->__writer_lock);
    }
    __unlock(&rwlock->__reader_lock);
    return 0;
}

int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock) {
    __lock(&rwlock->__writer_lock);
    return 0;
}
