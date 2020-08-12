#define __libc_internal

#include <bits/lock.h>
#include <pthread.h>
#include <stdatomic.h>
#include <sys/os_2.h>

void __lock_recursive(struct __recursive_lock *lock) {
    unsigned int tid = __get_self()->id;

    int expected = 0;
    int to_place = tid;

    while (!atomic_compare_exchange_strong(&lock->__lock, &expected, to_place)) {
        if ((expected & ~MUTEX_WAITERS) == tid) {
            lock->__count++;
            return;
        }

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
            to_place = MUTEX_WAITERS | tid;
        }

        os_mutex(&lock->__lock, MUTEX_AQUIRE, expected, to_place, 0, NULL);
        expected = 0;
    }

    lock->__count++;
}
