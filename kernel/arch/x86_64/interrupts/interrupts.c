#include "idt.h"
#include <kernel/interrupts/interrupts.h>

static struct idt_entry idt[NUM_INTERRUPTS];
static struct idt_descriptor idt_descriptor;

void handle_double_fault_entry();
void handle_page_fault_entry();

void init_interrupts() {
    idt_descriptor.idt = idt;
    idt_descriptor.limit = NUM_INTERRUPTS * sizeof(struct idt_entry) - 1;
    load_idt(idt_descriptor);
    add_idt_entry(idt, &handle_double_fault_entry, 8);
    add_idt_entry(idt, &handle_page_fault_entry, 14);
}