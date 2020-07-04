#include <stdlib.h>

#include <kernel/irqs/handlers.h>
#include <kernel/proc/task.h>

// #define GENERIC_IRQ_DEBUG

static struct irq_handler *irq_handlers[255];

struct irq_handler *create_irq_handler(void *handler, int flags, void *closure) {
    struct irq_handler *h = malloc(sizeof(struct irq_handler));
    h->handler = handler;
    h->flags = flags;
    h->closure = closure;
    h->next = NULL;
    return h;
}

void register_irq_handler(struct irq_handler *irq_handler_object, int irq_num) {
    assert(irq_num >= 0 && irq_num <= 255);

    irq_handler_object->next = irq_handlers[irq_num];
    irq_handlers[irq_num] = irq_handler_object;

    // FIXME: support potentially overlapping irq mappings
    assert(irq_handler_object->next == NULL);
}

void generic_irq_handler(int irq_number, struct task_state *task_state, uint32_t error_code) {
    (void) task_state;
    (void) error_code;

#ifdef GENERIC_IRQ_DEBUG
    debug_log("Got IRQ: [ %d ]\n", irq_number);
#endif /* GENERIC_IRQ_DEBUG */

    struct irq_handler *handler = irq_handlers[irq_number];
    if (!handler) {
        debug_log("No IRQ handler registered: [ %d ]\n", irq_number);
        return;
    }

    if ((handler->flags & IRQ_HANDLER_USE_TASK_STATE) && (handler->flags & IRQ_HANDLER_USE_ERROR_CODE)) {
        void (*typed_handler)(struct task_state * task_state, uint32_t error_code) = handler->handler;
        typed_handler(task_state, error_code);
        return;
    }
    assert(!(handler->flags & IRQ_HANDLER_USE_ERROR_CODE));

    if (handler->flags & IRQ_HANDLER_USE_TASK_STATE) {
        void (*typed_handler)(struct task_state * task_state) = handler->handler;
        typed_handler(task_state);
        return;
    }

    void (*typed_handler)(void) = handler->handler;
    typed_handler();
    return;
}
