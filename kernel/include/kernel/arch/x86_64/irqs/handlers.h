#ifndef _KERNEL_ARCH_X86_64_IRQS_HANDLERS_H
#define _KERNEL_ARCH_X86_64_IRQS_HANDLERS_H 1

#include <stdint.h>

#include <kernel/proc/process.h>

void handle_invalid_opcode_entry();
void handle_fpu_exception_entry();
void handle_device_not_available_entry();
void handle_double_fault_entry();
void handle_general_protection_fault_entry();
void handle_page_fault_entry();

void sys_call_entry();

void handle_double_fault();
void handle_general_protection_fault(struct stack_state *stack, uintptr_t error);
void handle_page_fault(struct stack_state *stack, uintptr_t address, uintptr_t error);
void handle_invalid_opcode();
void handle_fpu_exception();
void handle_device_not_available();

#endif /* _KERNEL_ARCH_X86_64_IRQS_HANDLERS_H */