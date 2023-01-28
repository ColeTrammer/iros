.global longjmp
.type longjmp, @function
longjmp:
    # jmp_buf passed in 4(%esp)
    # val passed in 8(%esp)
    mov 4(%esp), %edx
    mov 8(%esp), %eax

    mov 0(%edx), %ebx
    mov 4(%edx), %esi
    mov 8(%edx), %edi
    mov 12(%edx), %ebp
    mov 16(%edx), %esp

    test %eax, %eax
    jnz skip_fix_val
    movl $1, %eax

skip_fix_val:
    jmp *20(%edx)
