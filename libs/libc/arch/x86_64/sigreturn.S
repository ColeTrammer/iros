#include <sys/syscall.h>

.section .text
.global __sigreturn
__sigreturn:
    movq $SC_SIGRETURN, %rax
    syscall
    jmp __halt
