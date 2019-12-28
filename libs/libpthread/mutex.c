#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>
#include <string.h>
#include <sys/os_2.h>
#include <sys/syscall.h>

int pthread_mutex_init(pthread_mutex_t *__restrict mutex, const pthread_mutexattr_t *__restrict mutexattr) {
    if (mutex == NULL) {
        return EINVAL;
    }

    mutex->__lock = 0;
    mutex->__count_if_recursive = 0;
    if (mutexattr != NULL) {
        memcpy(&mutex->__attr, mutexattr, sizeof(pthread_mutexattr_t));
    } else {
        pthread_mutexattr_init(&mutex->__attr);
    }
    return 0;
}

int pthread_mutex_lock(pthread_mutex_t *mutex) {
    int expected = 0;
    pthread_t tid = pthread_self();
    while (!atomic_compare_exchange_strong(&mutex->__lock, &expected, tid)) {
        // Failed to aquire the lock
        if ((mutex->__attr.__flags & PTHREAD_MUTEX_ERRORCHECK) && (expected == tid)) {
            return EDEADLK;
        } else if ((mutex->__attr.__flags & PTHREAD_MUTEX_RECURSIVE) && (expected == tid)) {
            // Don't need any locks since only one thread owns this
            mutex->__count_if_recursive++;
            return 0;
        }

        syscall(SC_OS_MUTEX, &mutex->__lock, MUTEX_AQUIRE, expected, tid, 0, NULL);
        expected = 0;
    }

    return 0;
}

int pthread_mutex_trylock(pthread_mutex_t *mutex) {
    int expected = 0;
    pthread_t tid = pthread_self();
    if (!atomic_compare_exchange_strong(&mutex->__lock, &expected, tid)) {
        if ((mutex->__attr.__flags & PTHREAD_MUTEX_RECURSIVE) && (expected == tid)) {
            mutex->__count_if_recursive++;
            return 0;
        }

        // Failed to aquire the lock
        return EBUSY;
    }

    return 0;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex) {
    pthread_t tid = pthread_self();
    if ((mutex->__attr.__flags & PTHREAD_MUTEX_RECURSIVE) || (mutex->__attr.__flags & PTHREAD_MUTEX_ERRORCHECK)) {
        if (mutex->__lock != tid) {
            return EPERM;
        }

        if ((mutex->__attr.__flags & PTHREAD_MUTEX_RECURSIVE) && --mutex->__count_if_recursive != 0) {
            return 0;
        }
    }
    return syscall(SC_OS_MUTEX, &mutex->__lock, MUTEX_WAKE_AND_SET, pthread_self(), 0, 1, NULL);
}

int pthread_mutex_destroy(pthread_mutex_t *mutex) {
    if (mutex == NULL || mutex->__lock == -1) {
        return EINVAL;
    }

    mutex->__lock = -1;
    return 0;
}