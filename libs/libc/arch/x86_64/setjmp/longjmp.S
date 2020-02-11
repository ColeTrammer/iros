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