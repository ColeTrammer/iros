#ifndef _KERNEL_IRQS_HANDLERS_H
#define _KERNEL_IRQS_HANDLERS_H 1

#include <stdint.h>

// clang-format off
#include <kernel/arch/arch.h>
#include ARCH_SPECIFIC(irqs/arch_handlers.h)
// clang-format on

void init_irq_handlers(void);

#endif /* _KERNEL_IRQS_HANDLERS_H */
