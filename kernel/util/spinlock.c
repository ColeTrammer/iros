#include <kernel/util/spinlock.h>

#include <kernel/proc/process.h>
#include <kernel/hal/output.h>
#include <kernel/hal/hal.h>

void spin_lock(spinlock_t *lock) {
    while (1) {
        disable_interrupts();
        if (!xchg_32(lock, 1)) return;
        enable_interrupts();

        while (*lock) cpu_relax();
    }
}

void spin_unlock(spinlock_t *lock) {
    barrier();
    *lock = 0;
}