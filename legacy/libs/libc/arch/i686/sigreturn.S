#include <sys/syscall.h>

.section .text
.global __sigreturn
__sigreturn:
    # Remove pushed arguments from the stack
    # FIXME: this signal trampoline mechanism should be in
    #        the kernel...
    add $20, %esp
    mov $SYS_SIGRETURN, %eax
    int $0x80
    jmp __halt
