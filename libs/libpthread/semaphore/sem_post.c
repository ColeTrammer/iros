#define __libc_internal

#include <errno.h>
#include <limits.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <sys/os_2.h>

int sem_post(sem_t *s) {
    int expected = 0;
    int to_place = 1;
    for (;;) {
        if (atomic_compare_exchange_strong(s->__sem_value, &expected, to_place)) {
            return 0;
        }

        if (expected & MUTEX_WAITERS) {
            if (os_mutex((unsigned int *) s->__sem_value, MUTEX_WAKE_AND_SET, expected, expected + 1, 1, NULL)) {
                expected = 0;
                continue;
            }
            return 0;
        }

        if (expected == SEM_VALUE_MAX) {
            errno = EOVERFLOW;
            return -1;
        }
        to_place = expected + 1;
    }
}
