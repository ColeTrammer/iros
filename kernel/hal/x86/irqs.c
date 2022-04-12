#include <stdbool.h>
#include <string.h>

#include <kernel/hal/arch.h>
#include <kernel/hal/output.h>
#include <kernel/irqs/handlers.h>

// clang-format off
#include HAL_ARCH_SPECIFIC(idt.h)
// clang-format off

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

struct idt_descriptor *get_idt_descriptor(void) {
    return &idt_descriptor;
}
