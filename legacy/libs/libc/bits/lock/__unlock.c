#include <bits/lock.h>
#include <stdatomic.h>
#include <stddef.h>
#include <sys/iros.h>

void __unlock(struct __lock *lock) {
    int expected = 1;
    int to_place = 0;
    if (!atomic_compare_exchange_strong(&lock->__lock, &expected, to_place)) {
        // This means there was waiters...
        os_mutex(&lock->__lock, MUTEX_WAKE_AND_SET, expected, MUTEX_WAITERS, 1, NULL);
    }
}
