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

    # Should Check To See If RAX Is Invalid
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
    .quad 0
    .quad arch_sys_exit
    .quad arch_sys_sbrk
    .quad arch_sys_fork
    .quad arch_sys_open
    .quad arch_sys_read
    .quad arch_sys_write
    .quad arch_sys_close
    .quad arch_sys_execve
    .quad arch_sys_waitpid
    .quad arch_sys_getpid
    .quad arch_sys_getcwd
    .quad arch_sys_chdir
    .quad arch_sys_stat
    .quad arch_sys_lseek
    .quad arch_sys_ioctl