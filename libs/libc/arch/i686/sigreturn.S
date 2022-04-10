#include <sys/syscall.h>

.section .text
.global __sigreturn
__sigreturn:
    mov $SYS_SIGRETURN, %eax
    int $0x80
    jmp __halt
