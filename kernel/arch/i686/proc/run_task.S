.section .text
.global __run_task
__run_task:
    # Simply loads task_state struct from stack to %edi,
    # Making sure to load %rdi last for obvious reasons
    mov 4(%esp), %edi

    movw 0(%edi), %dx
    mov %dx, %ds
    movw 2(%edi), %dx
    mov %dx, %es
    movw 4(%edi), %dx
    mov %dx, %fs
    movw 6(%edi), %dx
    mov %dx, %gs

    mov 8(%edi), %ebp
    mov 16(%edi), %esi
    mov 20(%edi), %edx
    mov 24(%edi), %ecx
    mov 28(%edi), %ebx
    mov 32(%edi), %eax

    push 36(%edi)
    push 40(%edi)
    push 44(%edi)
    push 48(%edi)
    push 52(%edi)

    mov 12(%edi), %edi

    iret
