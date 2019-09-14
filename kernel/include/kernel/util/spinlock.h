#ifndef _KERNEL_UTIL_SPINLOCK_H
#define _KERNEL_UTIL_SPINLOCK_H 1

#include <stdint.h>
#include <stdbool.h>

#include <kernel/arch/arch.h>
#include ARCH_SPECIFIC(asm_utils.h)

typedef struct {
    int counter;
    unsigned long interrupts;
} spinlock_t;

void spin_lock(spinlock_t *lock);
void spin_unlock(spinlock_t *lock);

#define SPINLOCK_INITIALIZER { .counter=0, .interrupts=0UL }

void init_spinlock(spinlock_t *lock);

#endif /* _KERNEL_UTIL_SPINLOCK_H */