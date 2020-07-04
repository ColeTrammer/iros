#ifndef _KERNEL_IRQS_HANDLERS_H
#define _KERNEL_IRQS_HANDLERS_H 1

#include <stdbool.h>
#include <stdint.h>

#define IRQ_HANDLER_EXTERNAL 1

// clang-format off
#include <kernel/arch/arch.h>
#include ARCH_SPECIFIC(irqs/arch_handlers.h)
// clang-format on

struct task_state;

struct irq_controller;

struct irq_controller_ops {
    bool (*is_valid_irq)(struct irq_controller *self, int irq_num);
    void (*send_eoi)(struct irq_controller *self, int irq_num);
    void (*set_irq_enabled)(struct irq_controller *self, int irq_num, bool enabled);
};

struct irq_controller {
    struct irq_controller *next;
    int irq_start;
    int irq_end;
    struct irq_controller_ops *ops;
    void *private_data;
};

struct irq_context {
    struct irq_controller *irq_controller;
    struct task_state *task_state;
    int irq_num;
    uint32_t error_code;
    void *closure;
};

typedef void (*irq_function_t)(struct irq_context *context);

struct irq_handler {
    irq_function_t handler;
    int flags;
    void *closure;
    struct irq_handler *next;
};

void arch_system_call_entry(struct irq_context *irq_context);

void register_irq_controller(struct irq_controller *controller);

struct irq_handler *create_irq_handler(irq_function_t function, int flags, void *closure);
void register_irq_handler(struct irq_handler *handler, int irq_num);

void init_irq_handlers(void);

#endif /* _KERNEL_IRQS_HANDLERS_H */
