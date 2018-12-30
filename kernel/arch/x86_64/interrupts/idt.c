#include <stdint.h>
#include "idt.h"

void add_idt_entry(struct idt_entry *idt, void *_function, int interrupt_number) {
    uint64_t function = (uint64_t) _function;
    idt[interrupt_number].addr_low = (uint16_t) function;
    idt[interrupt_number].target = 0x08;
    idt[interrupt_number].flags = 0x8E00; // Present, CPL 0, Trap Handler
    idt[interrupt_number].addr_mid = (uint16_t) (function >> 16);
    idt[interrupt_number].addr_high = (uint32_t) (function >> 32);
    idt[interrupt_number].reserved = 0;
}