#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <sys/os_2.h>

int pthread_mutex_init(pthread_mutex_t *__restrict mutex, const pthread_mutexattr_t *__restrict mutexattr) {
    (void) mutexattr;

    mutex->__lock = 0;
    return 0;
}

int pthread_mutex_lock(pthread_mutex_t *mutex) {
    int expected = 0;
    if (!atomic_compare_exchange_strong(&mutex->__lock, &expected, 1)) {
        // Failed to aquire the lock
        return os_mutex(&mutex->__lock, MUTEX_AQUIRE, 1, 1);
    }

    return 0;
}

int pthread_mutex_trylock(pthread_mutex_t *mutex) {
    int expected = 0;
    if (!atomic_compare_exchange_strong(&mutex->__lock, &expected, 1)) {
        // Failed to aquire the lock
        return EBUSY;
    }

    return 0;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex) {
    return os_mutex(&mutex->__lock, MUTEX_RELEASE, 1, 0);
}

int pthread_mutex_destroy(pthread_mutex_t *mutex) {
    mutex->__lock = -1;
    return 0;
}