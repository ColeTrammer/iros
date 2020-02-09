#include <stdatomic.h>
#include <sys/os_2.h>

int __trylock(unsigned int *lock) {
    int expected = 0;
    int to_place = 1;
    if (!atomic_compare_exchange_strong(lock, &expected, to_place)) {
        return 0;
    }

    return 1;
}