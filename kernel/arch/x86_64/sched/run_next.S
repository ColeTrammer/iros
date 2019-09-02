.section .text
.global sched_run_next_entry
sched_run_next_entry:
    # cli

    # push %rax
    # push %rbx
    # push %rcx
    # push %rdx
    # push %rsi
    # push %rdi
    # push %rbp
    # push %r8
    # push %r9
    # push %r10
    # push %r11
    # push %r12
    # push %r13
    # push %r14
    # push %r15

    # movq %rsp, %rdi
    callq arch_sched_run_next

    jmp __halt