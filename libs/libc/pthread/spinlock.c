#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>

#define CPU_RELAX() asm volatile("pause" : : :);

int pthread_spin_destroy(pthread_spinlock_t *lock) {
    lock->lock = -1;
    return 0;
}

int pthread_spin_init(pthread_spinlock_t *lock, int pshared) {
    // FIXME: do something with pshared
    (void) pshared;

    lock->lock = 0;
    return 0;
}

int pthread_spin_lock(pthread_spinlock_t *lock) {
    for (;;) {
        int expected = 0;
        if (!atomic_compare_exchange_strong(&lock->lock, &expected, 1)) {
            CPU_RELAX();
            continue;
        }

        break;
    }

    return 0;
}

int pthread_spin_trylock(pthread_spinlock_t *lock) {
    int expected = 0;
    if (!atomic_compare_exchange_strong(&lock->lock, &expected, 1)) {
        // Failed to aquire the lock
        return EBUSY;
    }

    return 0;
}

int pthread_spin_unlock(pthread_spinlock_t *lock) {
    lock->lock = 0;
    return 0;
}
