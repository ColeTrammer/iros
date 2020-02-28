#ifndef _KERNEL_INTERRUPTS_INTERRUPTS_H
#define _KERNEL_INTERRUPTS_INTERRUPTS_H 1

#include <stdint.h>

// clang-format off
#include <kernel/arch/arch.h>
#include ARCH_SPECIFIC(irqs/handlers.h)
// clang-format on

struct task;

void init_irq_handlers();

#endif /* _KERNEL_INTERRUPTS_INTERRUPTS_H */