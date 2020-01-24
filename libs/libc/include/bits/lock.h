#ifndef _BITS_LOCK_H
#define _BITS_LOCK_H 1

#include <bits/null.h>
#include <stdatomic.h>
#include <sys/os_2.h>

static inline void __lock(unsigned int *lock) {
    int expected = 0;
    int to_place = 1;

    while (!atomic_compare_exchange_strong(lock, &expected, to_place)) {
        if (!(expected & MUTEX_WAITERS)) {
            to_place = expected | MUTEX_WAITERS;
            if (!atomic_compare_exchange_strong(lock, &expected, to_place)) {
                expected = 0;
                continue;
            }

            expected |= MUTEX_WAITERS;
        }

        os_mutex(lock, MUTEX_AQUIRE, expected, to_place, 0, NULL);
        expected = 0;
    }
}

static inline int __trylock(unsigned int *lock) {
    int expected = 0;
    int to_place = 1;
    if (!atomic_compare_exchange_strong(lock, &expected, to_place)) {
        return 0;
    }

    return 1;
}

static inline void __unlock(unsigned int *lock) {
    int expected = 1;
    int to_place = 0;
    if (!atomic_compare_exchange_strong(lock, &expected, to_place)) {
        // This means there was waiters...
        os_mutex(lock, MUTEX_WAKE_AND_SET, expected, to_place, 1, NULL);
    }
}

#endif /* _BITS_LOCK_H */