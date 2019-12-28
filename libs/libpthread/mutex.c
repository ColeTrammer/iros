#define __libc_internal

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>
#include <string.h>
#include <sys/os_2.h>

int pthread_mutex_consistent(pthread_mutex_t *mutex) {
    if (!mutex || !(mutex->__attr.__flags & PTHREAD_MUTEX_ROBUST) || !(mutex->__attr.__flags & __PTHREAD_MUTEX_INCONSISTENT)) {
        return EINVAL;
    }

    // Maybe should be atomic since apparently there's no requirement that the mutex is locked
    mutex->__attr.__flags &= ~(__PTHREAD_MUTEX_INCONSISTENT);
    return 0;
}

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
        if ((mutex->__attr.__flags & PTHREAD_MUTEX_ROBUST) && (mutex->__attr.__flags & __PTHREAD_MUTEX_UNRECOVERABLE)) {
            return ENOTRECOVERABLE;
        }

        if ((mutex->__attr.__flags & PTHREAD_MUTEX_ERRORCHECK) && (expected == tid)) {
            return EDEADLK;
        } else if ((mutex->__attr.__flags & PTHREAD_MUTEX_RECURSIVE) && (expected == tid)) {
            // Don't need any locks since only one thread owns this
            mutex->__count_if_recursive++;
            return 0;
        }

        if (mutex->__attr.__flags & PTHREAD_MUTEX_ROBUST) {
            // FIXME: work with PSHARED mutexes
            int ret = tgkill(0, mutex->__lock, 0);
            if (ret != 0) {
                // This effectively means the thread that owns the lock is gone, so now try to lock it. Else continue.
                int _expected = 0;
                if (atomic_compare_exchange_strong(&mutex->__lock, &_expected, tid)) {
                    // Now we have the lock and know the owner died.
                    mutex->__attr.__flags |= __PTHREAD_MUTEX_INCONSISTENT;
                    return EOWNERDEAD;
                }
            }
        }

        os_mutex(&mutex->__lock, MUTEX_AQUIRE, expected, tid, 0, NULL);
        expected = 0;
    }

    return 0;
}

int pthread_mutex_trylock(pthread_mutex_t *mutex) {
    int expected = 0;
    pthread_t tid = pthread_self();
    if (!atomic_compare_exchange_strong(&mutex->__lock, &expected, tid)) {
        if ((mutex->__attr.__flags & PTHREAD_MUTEX_ROBUST) && (mutex->__attr.__flags & __PTHREAD_MUTEX_UNRECOVERABLE)) {
            return ENOTRECOVERABLE;
        }

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
    if ((mutex->__attr.__flags & PTHREAD_MUTEX_ROBUST) || (mutex->__attr.__flags & PTHREAD_MUTEX_RECURSIVE) ||
        (mutex->__attr.__flags & PTHREAD_MUTEX_ERRORCHECK)) {
        if (mutex->__lock != tid) {
            return EPERM;
        }
    }

    if (mutex->__attr.__flags & PTHREAD_MUTEX_ROBUST) {
        if (mutex->__attr.__flags & __PTHREAD_MUTEX_UNRECOVERABLE) {
            return 0;
        }

        if (mutex->__attr.__flags & __PTHREAD_MUTEX_INCONSISTENT) {
            mutex->__attr.__flags |= __PTHREAD_MUTEX_UNRECOVERABLE;

            // Wake all threads waiting so they'll return ENOTRECOVERABLE (store 1 so nobody can ever)
            // lock it again, since tid 1 is reserved for the kernel
            return os_mutex(&mutex->__lock, MUTEX_WAKE_AND_SET, tid, 1, INT_MAX, NULL);
        }
    }

    if ((mutex->__attr.__flags & PTHREAD_MUTEX_RECURSIVE) && --mutex->__count_if_recursive != 0) {
        return 0;
    }

    return os_mutex(&mutex->__lock, MUTEX_WAKE_AND_SET, tid, 0, 1, NULL);
}

int pthread_mutex_destroy(pthread_mutex_t *mutex) {
    if (mutex == NULL || mutex->__lock == -1) {
        return EINVAL;
    }

    mutex->__lock = -1;
    return 0;
}