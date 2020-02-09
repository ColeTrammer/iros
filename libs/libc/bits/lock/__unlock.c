#include <stdatomic.h>
#include <stddef.h>
#include <sys/os_2.h>

void __unlock(unsigned int *lock) {
    int expected = 1;
    int to_place = 0;
    if (!atomic_compare_exchange_strong(lock, &expected, to_place)) {
        // This means there was waiters...
        os_mutex(lock, MUTEX_WAKE_AND_SET, expected, MUTEX_WAITERS, 1, NULL);
    }
}