#define __libc_internal

#include <limits.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>
#include <sys/iros.h>

static void reset_once(pthread_once_t *once_control) {
    atomic_store(once_control, 0);
}

int pthread_once(pthread_once_t *once_control, void (*init_routine)(void)) {
    for (;;) {
        int expect = 0;
        if (atomic_compare_exchange_strong(once_control, &expect, __PTHREAD_ONCE_IN_PROGRESS) != 0) {
            if (__threads) {
                pthread_cleanup_push((void (*)(void *)) & reset_once, once_control);
                (*init_routine)();
                pthread_cleanup_pop(0);
            } else {
                (*init_routine)();
            }
            os_mutex(once_control, MUTEX_WAKE_AND_SET, __PTHREAD_ONCE_IN_PROGRESS, __PTHREAD_ONCE_FINISHED, INT_MAX, NULL);
            break;
        }

        if (expect == __PTHREAD_ONCE_FINISHED) {
            break;
        }

        os_mutex(once_control, MUTEX_AQUIRE, __PTHREAD_ONCE_IN_PROGRESS, 0, 0, NULL);
    }

    return 0;
}
