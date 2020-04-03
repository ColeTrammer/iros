.section .text
.global __do_syscall
.type __do_syscall, @function
__do_syscall:
    movq 8(%rsp), %r10
    int $0x80
    retq
.size __do_syscall, . - __do_syscall