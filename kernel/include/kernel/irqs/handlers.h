#ifndef _KERNEL_INTERRUPTS_INTERRUPTS_H
#define _KERNEL_INTERRUPTS_INTERRUPTS_H 1

#include <stdint.h>

void init_irq_handlers();

void handle_invalid_opcode_entry();
void handle_fpu_exception_entry();
void handle_device_not_available_entry();
void handle_double_fault_entry();
void handle_general_protection_fault_entry();
void handle_page_fault_entry();

void sys_call_entry();

void handle_double_fault();
void handle_general_protection_fault(uintptr_t error);
void handle_page_fault(uintptr_t address, uintptr_t error);
void handle_invalid_opcode();
void handle_fpu_exception();
void handle_device_not_available();

#endif /* _KERNEL_INTERRUPTS_INTERRUPTS_H */