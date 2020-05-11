#include <stdbool.h>
#include <string.h>

#include <kernel/hal/irqs.h>
#include <kernel/hal/output.h>
#include <kernel/hal/x86_64/idt.h>

static struct idt_entry idt[NUM_IRQS];
static struct idt_descriptor idt_descriptor;

void init_irqs() {
    idt_descriptor.idt = idt;
    idt_descriptor.limit = NUM_IRQS * sizeof(struct idt_entry) - 1;
    load_idt(idt_descriptor);
}

bool is_irq_registered(unsigned int irq) {
    return idt[irq].flags ? true : false;
}

void register_irq_handler(void *handler, unsigned int irq, bool is_user, bool needs_separate_stack) {
    add_idt_entry(idt, handler, irq, is_user, needs_separate_stack);

    debug_log("IRQ Handler Added: [ %#.2X, %s, %#.16lX ]\n", irq, is_user ? "true" : "false", (uintptr_t) handler);
}

void unregister_irq_handler(unsigned int irq) {
    remove_idt_entry(idt, irq);

    debug_log("IRQ Handler Removed: [ %.3d ]\n", irq);
}
