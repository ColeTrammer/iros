#define __libc_internal

#include <bits/lock.h>
#include <pthread.h>
#include <stdatomic.h>
#include <sys/iros.h>

int __trylock_recursive(struct __recursive_lock *lock) {
    unsigned int tid = __get_self()->id;

    int expected = 0;
    int to_place = tid;
    if (!atomic_compare_exchange_strong(&lock->__lock, &expected, to_place)) {
        if ((expected & ~MUTEX_WAITERS) == tid) {
            lock->__count++;
            return 1;
        }
        return 0;
    }

    lock->__count++;
    return 1;
}
