.section .text
.global sys_sched_run_next
sys_sched_run_next:
    # Switches Stack And Then calls arch_sched_run_next

    movabsq $KERNEL_VM_STACK_START, %rsp
    callq arch_sched_run_next

    # Should Not Be Reached
    jmp __halt

.global yield_signal
yield_signal:
    movabsq $KERNEL_VM_STACK_START, %rsp
    callq sched_run_next

    # Should Not Be Reached
    jmp __halt

.global __kernel_yield
.type __kernel_yield, @function
__kernel_yield:
    push %rbp
    mov %rsp, %rbp

    # Switches stack and then builds task_state struct on that stack

    # Save old rsp and provide rip for reentry
    movabsq $continue, %rax
    movq %rsp, %rdx

    pushq $0x10
    pushq %rdx
    pushfq
    pushq $0x08
    pushq %rax

    push %rax
    push %rbx
    push %rcx
    push %rdx
    push %rsi
    push %rdi
    push %rbp
    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15

    movq %rsp, %rdi

    cli
    movabsq $KERNEL_VM_STACK_START, %rsp

    callq arch_sched_run_next

    # Should not be reached
    jmp __halt

continue:
    pop %rbp
    retq
.size __kernel_yield, . - __kernel_yield
