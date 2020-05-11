.section .text
.global __run_task
__run_task:
    # Simply loads task_state struct from pointer in %rdi,
    # Making sure to load %rdi last for obvious reasons
    movq (%rdi), %r15
    movq 8(%rdi), %r14
    movq 16(%rdi), %r13
    movq 24(%rdi), %r12
    movq 32(%rdi), %r11
    movq 40(%rdi), %r10
    movq 48(%rdi), %r9
    movq 56(%rdi), %r8
    movq 64(%rdi), %rbp

    movq 80(%rdi), %rsi
    movq 88(%rdi), %rdx
    movq 96(%rdi), %rcx
    movq 104(%rdi), %rbx
    movq 112(%rdi), %rax

    pushq 152(%rdi)
    pushq 144(%rdi)
    pushq 136(%rdi)
    pushq 128(%rdi)
    pushq 120(%rdi)

    movq 72(%rdi), %rdi

    iretq
