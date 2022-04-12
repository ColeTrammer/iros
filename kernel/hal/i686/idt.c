#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include <kernel/hal/i686/gdt.h>
#include <kernel/hal/i686/idt.h>
#include <kernel/irqs/handlers.h>

void add_idt_entry(struct idt_entry *idt, void *_handler, unsigned int irq, int flags) {
    uintptr_t handler = (uintptr_t) _handler;
    idt[irq].addr_low = (uint16_t) handler;
    idt[irq].target = CS_SELECTOR;
    if (!(flags & IRQ_USER_AVAILABLE)) {
        idt[irq].flags = 0x8E00; // Present, CPL 0, Trap Handler
    } else {
        assert(!(flags & IRQ_USE_SEPARATE_STACK));
        assert(irq == 128);
        idt[irq].flags = 0xEE00;
    }
    idt[irq].addr_high = (uint16_t) (handler >> 16);
}

void remove_idt_entry(struct idt_entry *idt, unsigned int irq) {
    idt[irq].flags = 0; // Clears Present
}
