#include <kernel/hal/i686/gdt.h>

.section .text
.global __kernel_yield
.type __kernel_yield, @function
__kernel_yield:
    push %ebp
    mov %esp, %ebp

    # Switches stack and then builds task_state struct on that stack

    # Save old rsp
    mov %esp, %eax

    push $DATA_SELECTOR
    push %eax
    pushf
    push $CS_SELECTOR
    push $continue

    push %eax
    push %ebx
    push %ecx
    push %edx
    push %esi
    push %edi
    push %ebp

    pushw %gs
    pushw %fs
    pushw %es
    pushw %ds

    # Set up argument to scheduler
    mov %esp, %eax
    sub $0x4, %esp
    and $0xFFFFFFF0, %esp
    mov %eax, 0(%esp)

    cli
    call arch_sched_run_next

    # Should not be reached
    jmp __halt

continue:
    pop %ebp
    ret
.size __kernel_yield, . - __kernel_yield
