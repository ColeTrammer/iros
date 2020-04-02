#ifndef _KERNEL_ARCH_X86_64_IRQS_ARCH_HANDLERS_H
#define _KERNEL_ARCH_X86_64_IRQS_ARCH_HANDLERS_H 1

#include <stdint.h>

#include <kernel/proc/task.h>

struct task_interrupt_state {
    struct cpu_state cpu_state;
    uint64_t error_code;
    struct stack_state stack_state;
} __attribute__((packed));

void handle_divide_by_zero_entry();
void handle_invalid_opcode_entry();
void handle_fpu_exception_entry();
void handle_device_not_available_entry();
void handle_double_fault_entry();
void handle_stack_fault_entry();
void handle_general_protection_fault_entry();
void handle_page_fault_entry();

void sys_call_entry();

void handle_divide_by_zero(struct task_state *task_state);
void handle_double_fault();
void handle_stack_fault();
void handle_general_protection_fault(struct task_interrupt_state *task_state);
void handle_page_fault(struct task_interrupt_state *task_state, uintptr_t address);
void handle_invalid_opcode();
void handle_fpu_exception();
void handle_device_not_available();

#endif /* _KERNEL_ARCH_X86_64_IRQS_ARCH_HANDLERS_H */