#include "idt.h"
#include <kernel/interrupts/interrupts.h>

static struct idt_entry idt[NUM_INTERRUPTS];
static struct idt_descriptor idt_descriptor;

void __handle_double_fault();

void init_interrupts() {
    idt_descriptor.idt = idt;
    idt_descriptor.limit = NUM_INTERRUPTS * sizeof(struct idt_entry) - 1;
    load_idt(idt_descriptor);
    add_idt_entry(idt, &__handle_double_fault, 8);
}