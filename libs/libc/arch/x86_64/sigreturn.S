#include <sys/syscall.h>

.section .text
.global __sigreturn
__sigreturn:
    movq $SC_SIGRETURN, %rdi
    int $0x80
    jmpq __halt