#include <stdbool.h>
#include <string.h>

#include <kernel/hal/output.h>
#include <kernel/hal/x86_64/idt.h>
#include <kernel/irqs/handlers.h>

static struct idt_entry idt[NUM_IRQS];
static struct idt_descriptor idt_descriptor;

void init_irqs() {
#undef __ENUMERATE_IRQ
#define __ENUMERATE_IRQ(n, f) add_idt_entry(idt, &interrupt_handler_##n, n, f);
    __ENUMERATE_IRQS

    idt_descriptor.idt = idt;
    idt_descriptor.limit = NUM_IRQS * sizeof(struct idt_entry) - 1;
    load_idt(idt_descriptor);
}
