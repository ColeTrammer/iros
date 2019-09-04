#ifndef _KERNEL_UTIL_SPINLOCK_H
#define _KERNEL_UTIL_SPINLOCK_H 1

#include <stdint.h>

#include <kernel/arch/arch.h>
#include ARCH_SPECIFIC(asm_utils.h)

typedef uint32_t spinlock_t;

void spin_lock(spinlock_t *lock);
void spin_unlock(spinlock_t *lock);

#define SPINLOCK_INITIALIZER 0

#endif /* _KERNEL_UTIL_SPINLOCK_H */