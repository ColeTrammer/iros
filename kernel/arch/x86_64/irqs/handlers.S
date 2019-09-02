.section .text
.global handle_double_fault_entry
handle_double_fault_entry:
    call handle_double_fault
    iretq

.global handle_general_protection_fault_entry
handle_general_protection_fault_entry:
    movq (%rsp), %rdi
    call handle_general_protection_fault
    add $8, %rsp
    iretq

.global handle_page_fault_entry
handle_page_fault_entry:
    mov %cr2, %rdi
    movq (%rsp), %rsi
    call handle_page_fault
    add $8, %rsp
    iretq

.global sys_call_entry
sys_call_entry:
    cli

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

    movabs $sys_call_table, %rax
    movq (%rax, %rdi, 8), %rax
    movq %rsp, %rdi
    callq *%rax
    
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

.section .rodata
sys_call_table:
    .quad arch_sys_print
    .quad arch_sys_exit