#ifndef _KERNEL_INTERRUPTS_INTERRUPTS_H
#define _KERNEL_INTERRUPTS_INTERRUPTS_H 1

#include <stdint.h>

void init_irq_handlers();

void handle_double_fault_entry();
void handle_general_protection_fault_entry();
void handle_page_fault_entry();

void handle_double_fault();
void handle_general_protection_fault(uintptr_t error);
void handle_page_fault(uintptr_t address, uintptr_t error);

#endif /* _KERNEL_INTERRUPTS_INTERRUPTS_H */