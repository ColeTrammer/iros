#include <bits/lock.h>
#include <stdatomic.h>
#include <stddef.h>
#include <sys/iros.h>

void __lock(struct __lock *lock) {
    int expected = 0;
    int to_place = 1;

    while (!atomic_compare_exchange_strong(&lock->__lock, &expected, to_place)) {
        if ((expected & MUTEX_WAITERS) && !(expected & ~MUTEX_WAITERS)) {
            to_place |= MUTEX_WAITERS;
            continue;
        }

        if (!(expected & MUTEX_WAITERS)) {
            to_place = expected | MUTEX_WAITERS;
            if (!atomic_compare_exchange_strong(&lock->__lock, &expected, to_place)) {
                expected = 0;
                continue;
            }

            expected |= MUTEX_WAITERS;
            to_place = MUTEX_WAITERS | 1;
        }

        os_mutex(&lock->__lock, MUTEX_AQUIRE, expected, to_place, 0, NULL);
        expected = 0;
    }
}
