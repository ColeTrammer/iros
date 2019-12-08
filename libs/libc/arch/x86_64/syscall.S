#include <sys/syscall.h>

.section .text
.global __sigreturn
__sigreturn:
    movq $SC_SIGRETURN, %rdi
    int $0x80
    jmpq __halt

.global __do_syscall
__do_syscall:
    movq 8(%rsp), %r10
    int $0x80
    retq