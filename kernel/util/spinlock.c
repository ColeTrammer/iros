#include <kernel/util/spinlock.h>

#include <kernel/proc/process.h>
#include <kernel/hal/output.h>
#include <kernel/hal/hal.h>

void spin_lock(spinlock_t *lock) {
    while (1) {
        unsigned long interrupts = disable_interrupts_save();
        if (!xchg_32(&lock->counter, 1)) {
            lock->interrupts = interrupts;
            return;
        }
        enable_interrupts();
        
        while (lock->counter) cpu_relax();
    }
}

void spin_unlock(spinlock_t *lock) {
    unsigned long interrupts = lock->interrupts;
    if (interrupts & (1 << 9)) {
        barrier();
        lock->counter = 0;
        enable_interrupts();
    } else {
        barrier();
        lock->counter = 0;
    }
}