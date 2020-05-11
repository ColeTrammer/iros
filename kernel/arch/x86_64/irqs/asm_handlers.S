.section .text
.global handle_debug_entry
handle_debug_entry:
    call handle_debug
    iretq

.global handle_non_maskable_interrupt_entry
handle_non_maskable_interrupt_entry:
    call handle_non_maskable_interrupt
    iretq
    
.global handle_breakpoint_entry
handle_breakpoint_entry:
    call handle_breakpoint
    iretq

.global handle_overflow_entry
handle_overflow_entry:
    call handle_overflow
    iretq
    
.global handle_bound_range_exceeded_entry
handle_bound_range_exceeded_entry:
    call handle_bound_range_exceeded
    iretq
    
.global handle_invalid_tss_entry
handle_invalid_tss_entry:
    call handle_invalid_tss
    iretq
    
.global handle_segment_not_present_entry
handle_segment_not_present_entry:
    call handle_segment_not_present
    iretq
    
.global handle_alignment_check_entry
handle_alignment_check_entry:
    call handle_alignment_check
    iretq
    
.global handle_machine_check_entry
handle_machine_check_entry:
    call handle_machine_check
    iretq
    
.global handle_simd_exception_entry
handle_simd_exception_entry:
    call handle_simd_exception
    iretq
    
.global handle_virtualization_exception_entry
handle_virtualization_exception_entry:
    call handle_virtualization_exception
    iretq
    
.global handle_security_exception_entry
handle_security_exception_entry:
    call handle_security_exception
    iretq

.global handle_double_fault_entry
handle_double_fault_entry:
    call handle_double_fault
    iretq

.global handle_invalid_opcode_entry
handle_invalid_opcode_entry:
    call handle_invalid_opcode
    iretq

.global handle_fpu_exception_entry
handle_fpu_exception_entry:
    call handle_fpu_exception
    iretq

.global handle_divide_by_zero_entry
handle_divide_by_zero_entry:
    push %rax
    push %rbx
    push %rcx
    push %rdx
    push %rsi
    push %rdi
    push %rbp
    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15

    mov %rsp, %rdi
    call handle_divide_by_zero

    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rbp
    pop %rdi
    pop %rsi
    pop %rdx
    pop %rcx
    pop %rbx
    pop %rax

    iretq

.global handle_device_not_available_entry
handle_device_not_available_entry:
    push %rax
    push %rbx
    push %rcx
    push %rdx
    push %rsi
    push %rdi
    push %rbp
    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15

    call handle_device_not_available

    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rbp
    pop %rdi
    pop %rsi
    pop %rdx
    pop %rcx
    pop %rbx
    pop %rax

    iretq

.global handle_stack_fault_entry
handle_stack_fault_entry:
    push %rax
    push %rbx
    push %rcx
    push %rdx
    push %rsi
    push %rdi
    push %rbp
    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15

    mov %rsp, %rdi
    call handle_stack_fault

    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rbp
    pop %rdi
    pop %rsi
    pop %rdx
    pop %rcx
    pop %rbx
    pop %rax

    add $8, %rsp
    iretq

.global handle_general_protection_fault_entry
handle_general_protection_fault_entry:
    push %rax
    push %rbx
    push %rcx
    push %rdx
    push %rsi
    push %rdi
    push %rbp
    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15

    mov %rsp, %rdi
    call handle_general_protection_fault

    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rbp
    pop %rdi
    pop %rsi
    pop %rdx
    pop %rcx
    pop %rbx
    pop %rax

    add $8, %rsp
    iretq

.global handle_page_fault_entry
handle_page_fault_entry:
    push %rax
    push %rbx
    push %rcx
    push %rdx
    push %rsi
    push %rdi
    push %rbp
    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15

    mov %rsp, %rdi
    mov %cr2, %rsi
    movq 120(%rsp), %rdx
    call handle_page_fault

    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rbp
    pop %rdi
    pop %rsi
    pop %rdx
    pop %rcx
    pop %rbx
    pop %rax

    add $8, %rsp
    iretq

.global sys_call_entry
.type sys_call_entry, @function
sys_call_entry:
    push %rax
    push %rbx
    push %rcx
    push %rdx
    push %rsi
    push %rdi
    push %rbp
    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15

    mov %rsp, %rdi
    callq arch_system_call_entry
    
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rbp
    pop %rdi
    pop %rsi
    pop %rdx
    pop %rcx
    pop %rbx
    pop %rax

    iretq
.size sys_call_entry, . - sys_call_entry
