.section .text
.global sigsetjmp
.type sigsetjmp, @function
sigsetjmp:
    # jmp_buf passed starting at 4(%esp)
    mov 4(%esp), %eax

    mov %ebx, 0(%eax)
    mov %esi, 4(%eax)
    mov %edi, 8(%eax)
    mov %ebp, 12(%eax)

    # save esp
    lea 4(%esp), %edx
    mov %edx, 16(%eax)

    # save eip
    mov (%esp), %edx
    mov %edx, 20(%eax)

    # Go to part that handles signals
    jmp __sigsetjmp
