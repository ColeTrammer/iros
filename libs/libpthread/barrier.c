#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdatomic.h>
#include <sys/os_2.h>

int pthread_barrier_destroy(pthread_barrier_t *barrier) {
    if (barrier == NULL || barrier->__count_gap == 0) {
        return EINVAL;
    }

    barrier->__count_gap = 0;
    return 0;
}

int pthread_barrier_init(pthread_barrier_t *__restrict barrier, const pthread_barrierattr_t *__restrict attr, unsigned int count) {
    if (!barrier || count == 0) {
        return EINVAL;
    }

    if (attr) {
        barrier->__attr = *attr;
    }

    barrier->__count_now = 0;
    barrier->__count_to = count;
    barrier->__count_gap = count;
    return 0;
}

int pthread_barrier_wait(pthread_barrier_t *barrier) {
    unsigned int count_gap = barrier->__count_gap;

    unsigned int count_to = atomic_load(&barrier->__count_to);
    unsigned int count_now = 1 + atomic_fetch_add(&barrier->__count_now, 1);
    while (count_now > count_to) {
        // Wait for the correct counter value to be loaded.
        count_to = atomic_load(&barrier->__count_to);
    }

    if (count_now == count_to) {
        os_mutex(&barrier->__count_to, MUTEX_WAKE_AND_SET, count_to, count_to + count_gap, INT_MAX, NULL);
        return PTHREAD_BARRIER_SERIAL_THREAD;
    }

    while (count_now + count_gap > count_to) {
        os_mutex(&barrier->__count_to, MUTEX_AQUIRE, count_to, 0, 0, NULL);
        count_to = atomic_load(&barrier->__count_to);
    }
    return 0;
}
