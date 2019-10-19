#ifndef _KERNEL_INTERRUPTS_INTERRUPTS_H
#define _KERNEL_INTERRUPTS_INTERRUPTS_H 1

#include <stdint.h>

#include <kernel/arch/arch.h>
#include ARCH_SPECIFIC(irqs/handlers.h)

struct process;

void init_irq_handlers();

void invalidate_last_saved(struct process *process);

#endif /* _KERNEL_INTERRUPTS_INTERRUPTS_H */