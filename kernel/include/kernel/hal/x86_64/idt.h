#ifndef _KERNEL_HAL_X86_64_IDT_H
#define _KERNEL_HAL_X86_64_IDT_H 1

#include <stdbool.h>
#include <stdint.h>

#define NUM_IRQS 256

struct idt_entry {
    uint16_t addr_low;
    uint16_t target;
    uint16_t flags;
    uint16_t addr_mid;
    uint32_t addr_high;
    uint32_t reserved;
} __attribute__((packed));

struct idt_descriptor {
    uint16_t limit;
    struct idt_entry *idt;
} __attribute__((packed));

static inline void load_idt(struct idt_descriptor descriptor) {
    asm("lidtq %0" : : "m"(descriptor));
}

void add_idt_entry(struct idt_entry *idt, void *handler, unsigned int irq, int flags);
void remove_idt_entry(struct idt_entry *idt, unsigned int irq);

#endif /* _KERNEL_HAL_X86_64_IDT_H */
