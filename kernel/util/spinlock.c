#include <kernel/util/spinlock.h>

#include <kernel/hal/hal.h>
#include <kernel/hal/output.h>
#include <kernel/proc/task.h>

void init_spinlock(spinlock_t *lock) {
    lock->counter = 0;
    lock->interrupts = 0;
}

void spin_lock(spinlock_t *lock) {
    while (1) {
        unsigned long interrupts = disable_interrupts_save();
        if (!xchg_32(&lock->counter, 1)) {
            lock->interrupts = interrupts;
            return;
        }
        enable_interrupts();

        while (lock->counter)
            cpu_relax();
    }
}

void spin_unlock(spinlock_t *lock) {
    unsigned long interrupts = lock->interrupts;
    barrier();
    lock->counter = 0;
    interrupts_restore(interrupts);
}