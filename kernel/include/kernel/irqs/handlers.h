#ifndef _KERNEL_IRQS_HANDLERS_H
#define _KERNEL_IRQS_HANDLERS_H 1

#include <stdint.h>

#define IRQ_HANDLER_EXTERNAL       1
#define IRQ_HANDLER_USE_TASK_STATE 2
#define IRQ_HANDLER_USE_ERROR_CODE 4

// clang-format off
#include <kernel/arch/arch.h>
#include ARCH_SPECIFIC(irqs/arch_handlers.h)
// clang-format on

struct task_state;

struct irq_handler {
    void *handler;
    int flags;
    void *closure;
    struct irq_handler *next;
};

void arch_system_call_entry(struct task_state *task_state);

void init_irq_handlers(void);

struct irq_handler *create_irq_handler(void *handler, int flags, void *closure);
void register_irq_handler(struct irq_handler *handler, int irq_num);

#endif /* _KERNEL_IRQS_HANDLERS_H */
