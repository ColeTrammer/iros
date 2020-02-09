#include <stdatomic.h>
#include <stddef.h>
#include <sys/os_2.h>

void __lock(unsigned int *lock) {
    int expected = 0;
    int to_place = 1;

    while (!atomic_compare_exchange_strong(lock, &expected, to_place)) {
        if ((expected & MUTEX_WAITERS) && !(expected & ~MUTEX_WAITERS)) {
            to_place |= MUTEX_WAITERS;
            continue;
        }

        if (!(expected & MUTEX_WAITERS)) {
            to_place = expected | MUTEX_WAITERS;
            if (!atomic_compare_exchange_strong(lock, &expected, to_place)) {
                expected = 0;
                continue;
            }

            expected |= MUTEX_WAITERS;
            to_place = MUTEX_WAITERS | 1;
        }

        os_mutex(lock, MUTEX_AQUIRE, expected, to_place, 0, NULL);
        expected = 0;
    }
}