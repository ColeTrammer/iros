#ifndef _KERNEL_UTIL_SPINLOCK_H
#define _KERNEL_UTIL_SPINLOCK_H 1

#include <stdbool.h>
#include <stdint.h>

#include <kernel/arch/arch.h>
#include ARCH_SPECIFIC(asm_utils.h)

typedef struct {
    int counter;
    unsigned long interrupts;
} spinlock_t;

void spin_lock_internal(spinlock_t *lock, const char *func, bool handle_messages);
void spin_unlock_internal(spinlock_t *lock, const char *func, bool irq_resore);

#define SPINLOCK_INITIALIZER \
    { .counter = 0, .interrupts = 0UL }

void init_spinlock_internal(spinlock_t *lock, const char *func);

#define spin_lock(lock)                  spin_lock_internal(lock, __func__, true)
#define spin_unlock(lock)                spin_unlock_internal(lock, __func__, false)
#define spin_unlock_no_irq_restore(lock) spin_unlock_internal(lock, __func__, true)
#define init_spinlock(lock)              init_spinlock_internal(lock, __func__)

#endif /* _KERNEL_UTIL_SPINLOCK_H */
