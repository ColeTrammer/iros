#define __libc_internal

#include <bits/lock.h>
#include <pthread.h>
#include <stdatomic.h>
#include <sys/os_2.h>

void __unlock_recursive(struct __recursive_lock *lock) {
    if (!--lock->__count) {
        int tid = __get_self()->id;
        int expected = tid;
        int to_place = 0;
        if (!atomic_compare_exchange_strong(&lock->__lock, &expected, to_place)) {
            // This means there was waiters...
            os_mutex(&lock->__lock, MUTEX_WAKE_AND_SET, expected, MUTEX_WAITERS, 1, NULL);
        }
    }
}
