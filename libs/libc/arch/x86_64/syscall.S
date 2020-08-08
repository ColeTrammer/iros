.section .text
.global __do_syscall
.type __do_syscall, @function
__do_syscall:
    mov %rdi, %rax
    mov %rsi, %rdi
    mov %rdx, %rsi
    mov %rcx, %rdx
    mov %r8, %rcx
    mov %r9, %r8
    movq 8(%rsp), %r9
    int $0x80
    retq
.size __do_syscall, . - __do_syscall
