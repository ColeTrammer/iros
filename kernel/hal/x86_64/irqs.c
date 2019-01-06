#include "idt.h"

#include <hal/irqs.h>

static struct idt_entry idt[NUM_IRQS];
static struct idt_descriptor idt_descriptor;

void init_irqs() {
    idt_descriptor.idt = idt;
    idt_descriptor.limit = NUM_IRQS * sizeof(struct idt_entry) - 1;
    load_idt(idt_descriptor);
}

void register_irq_handler(void *handler, unsigned int irq) {
    add_idt_entry(idt, handler, irq);
}

void unregister_irq_handler(unsigned int irq) {
    remove_idt_entry(idt, irq);
}