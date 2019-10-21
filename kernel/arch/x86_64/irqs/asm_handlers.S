.section .text
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

    # Manually clear cr0.TS so that we can mess with fpu state
    clts

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

.global handle_general_protection_fault_entry
handle_general_protection_fault_entry:
    lea -8(%rsp), %rdi
    mov (%rsp), %rsi
    call handle_general_protection_fault
    add $8, %rsp
    iretq

.global handle_page_fault_entry
handle_page_fault_entry:
    lea -8(%rsp), %rdi
    mov %cr2, %rsi
    movq (%rsp), %rdx
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
    .quad arch_sys_ftruncate
    .quad arch_sys_time
    .quad arch_sys_mkdir
    .quad arch_sys_dup2
    .quad arch_sys_pipe
    .quad arch_sys_unlink
    .quad arch_sys_rmdir
    .quad arch_sys_chmod
    .quad arch_sys_kill
    .quad arch_sys_setpgid
    .quad arch_sys_sigaction
    .quad arch_sys_sigreturn
    .quad arch_sys_sigprocmask
    .quad arch_sys_dup