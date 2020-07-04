#include <stdlib.h>

#include <kernel/irqs/handlers.h>
#include <kernel/proc/task.h>

// #define GENERIC_IRQ_DEBUG

static struct irq_handler *irq_handlers[255];
static struct irq_controller *controllers;

static struct irq_controller *find_irq_controller(int irq) {
    struct irq_controller *controller = controllers;
    while (controller) {
        if (irq >= controller->irq_start && irq <= controller->irq_end) {
            return controller;
        }
        controller = controller->next;
    }
    return NULL;
}

void register_irq_controller(struct irq_controller *controller) {
    int new_start = controller->irq_start;
    int new_end = controller->irq_end;

    struct irq_controller **element = &controllers;
    while (*element) {
        int elem_start = (*element)->irq_start;
        int elem_end = (*element)->irq_end;

        // Assert that the irq controllers don't overlap
        assert(!(new_start >= elem_start && new_start <= elem_end));
        assert(!(new_end >= elem_start && new_end <= elem_end));
        assert(!(elem_start >= new_start && elem_start <= new_end));
        assert(!(elem_end >= new_start && elem_end <= new_end));

        element = &(*element)->next;
    }
    *element = controller;
}

struct irq_handler *create_irq_handler(irq_function_t function, int flags, void *closure) {
    struct irq_handler *h = malloc(sizeof(struct irq_handler));
    h->handler = function;
    h->flags = flags;
    h->closure = closure;
    h->next = NULL;
    return h;
}

void register_irq_handler(struct irq_handler *irq_handler_object, int irq_num) {
    assert(irq_num >= 0 && irq_num <= 255);

    irq_handler_object->next = irq_handlers[irq_num];
    irq_handlers[irq_num] = irq_handler_object;

    struct irq_controller *controller = find_irq_controller(irq_num);
    if (irq_handler_object->flags & IRQ_HANDLER_EXTERNAL) {
        assert(irq_is_external(irq_num));
        assert(controller);

        controller->ops->set_irq_enabled(controller, irq_num, true);
    } else {
        assert(!irq_is_external(irq_num));
        assert(!controller);
    }

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

    struct irq_context context = {
        .closure = handler->closure, .error_code = error_code, .irq_num = irq_number, .task_state = task_state, .irq_controller = NULL
    };

    if (handler->flags & IRQ_HANDLER_EXTERNAL) {
        struct irq_controller *controller = find_irq_controller(irq_number);
        if (!controller) {
            debug_log("External IRQ has no controller: [ %d ]\n", irq_number);
            return;
        }

        if (!controller->ops->is_valid_irq(controller, irq_number)) {
            debug_log("External IRQ was invalid: [ %d ]\n", irq_number);
            return;
        }

        context.irq_controller = controller;
        handler->handler(&context);
        controller->ops->send_eoi(controller, irq_number);
    } else {
        handler->handler(&context);
    }
}
