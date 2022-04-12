.section .text
.global __kernel_yield
.type __kernel_yield, @function
__kernel_yield:
    push %ebp
    mov %esp, %ebp

    # Switches stack and then builds task_state struct on that stack

    # Save old rsp and provide rip for reentry
    mov $continue, %eax
    mov %esp, %edx

    push $0x10
    push %edx
    pushf
    push $0x08
    push %eax

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
    push %esp

    cli
    call arch_sched_run_next

    # Should not be reached
    jmp __halt

continue:
    pop %ebp
    ret
.size __kernel_yield, . - __kernel_yield
