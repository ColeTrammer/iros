#include <bits/lock.h>
#include <stdatomic.h>
#include <sys/iros.h>

int __trylock(struct __lock *lock) {
    int expected = 0;
    int to_place = 1;
    if (!atomic_compare_exchange_strong(&lock->__lock, &expected, to_place)) {
        return 0;
    }

    return 1;
}
