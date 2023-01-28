.section .text
.global __do_syscall
.type __do_syscall, @function
__do_syscall:
    mov %rdi, %rax
    mov %rsi, %rdi
    mov %rdx, %rsi
    mov %rcx, %rdx
    movq 8(%rsp), %r10
    syscall
    retq
.size __do_syscall, . - __do_syscall
