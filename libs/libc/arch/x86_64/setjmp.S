.section .text
.global setjmp
.type setjmp, @function
setjmp:
    # jmp_buf passed in %rdi

    movq %rbx, 0(%rdi)
    movq %rbp, 8(%rdi)
    movq %r12, 16(%rdi)
    movq %r13, 24(%rdi)
    movq %r14, 32(%rdi)
    movq %r15, 40(%rdi)

    leaq 8(%rsp), %rax
    movq %rax, 48(%rdi) # RSP
    
    movq (%rsp), %rax
    movq %rax, 56(%rdi) # RIP

    # Return 0
    xorl %eax, %eax
    ret

.global sigsetjmp
.type sigsetjmp, @function
sigsetjmp:
    # jmp_buf passed in %rdi

    movq %rbx, 0(%rdi)
    movq %rbp, 8(%rdi)
    movq %r12, 16(%rdi)
    movq %r13, 24(%rdi)
    movq %r14, 32(%rdi)
    movq %r15, 40(%rdi)

    leaq 8(%rsp), %rax
    movq %rax, 48(%rdi) # RSP
    
    movq (%rsp), %rax
    movq %rax, 56(%rdi) # RIP

    # Go to part that handles signals
    jmp __sigsetjmp

.global longjmp
.type longjmp, @function
longjmp:
    # jmp_buf passed in %rdi
    # val passed in %esi

    movq 0(%rdi), %rbx
    movq 8(%rdi), %rbp
    movq 16(%rdi), %r12
    movq 24(%rdi), %r13
    movq 32(%rdi), %r14
    movq 40(%rdi), %r15
    movq 48(%rdi), %rsp

    movl %esi, %eax
    
    test %eax, %eax
    jnz skip_fix_val
    movl $1, %eax

skip_fix_val:
    jmp *56(%rdi)