#define __libc_internal

#include <errno.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <sys/os_2.h>

int __sem_wait(struct __sem *__restrict s, const struct timespec *__restrict timeout, int try_only) {
    (void) timeout;

    int expected = atomic_load(s->__sem_value);
    for (;;) {
        if ((expected & ~MUTEX_WAITERS) == 0) {
            if (try_only) {
                errno = EAGAIN;
                return -1;
            }

            int to_place = expected | MUTEX_WAITERS;
            if (to_place == expected || atomic_compare_exchange_strong(s->__sem_value, &expected, to_place)) {
                os_mutex((unsigned int *) s->__sem_value, MUTEX_AQUIRE, expected, 0, 0, NULL);
                expected = atomic_load_explicit(s->__sem_value, memory_order_relaxed);
            }
            continue;
        }

        int to_place = expected - 1;
        if (atomic_compare_exchange_strong(s->__sem_value, &expected, to_place)) {
            return 0;
        }
        continue;
    }
}
