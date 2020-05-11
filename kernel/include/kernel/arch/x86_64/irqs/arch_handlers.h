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
void handle_debug_entry();
void handle_non_maskable_interrupt_entry();
void handle_breakpoint_entry();
void handle_overflow_entry();
void handle_bound_range_exceeded_entry();
void handle_invalid_opcode_entry();
void handle_device_not_available_entry();
void handle_double_fault_entry();
void handle_invalid_tss_entry();
void handle_segment_not_present_entry();
void handle_stack_fault_entry();
void handle_general_protection_fault_entry();
void handle_page_fault_entry();
void handle_fpu_exception_entry();
void handle_alignment_check_entry();
void handle_machine_check_entry();
void handle_simd_exception_entry();
void handle_virtualization_exception_entry();
void handle_security_exception_entry();

void sys_call_entry();

void handle_divide_by_zero(struct task_state *task_state);
void handle_debug();
void handle_non_maskable_interrupt();
void handle_breakpoint();
void handle_overflow();
void handle_bound_range_exceeded();
void handle_stack_fault();
void handle_double_fault();
void handle_general_protection_fault(struct task_interrupt_state *task_state);
void handle_page_fault(struct task_interrupt_state *task_state, uintptr_t address);
void handle_invalid_opcode();
void handle_invalid_tss();
void handle_segment_not_present();
void handle_fpu_exception();
void handle_device_not_available();
void handle_alignment_check();
void handle_machine_check();
void handle_simd_exception();
void handle_virtualization_exception();
void handle_security_exception();

#endif /* _KERNEL_ARCH_X86_64_IRQS_ARCH_HANDLERS_H */
