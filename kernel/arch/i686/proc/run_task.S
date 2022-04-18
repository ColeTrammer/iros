.section .text
.global __run_task
__run_task:
    # Simply loads task_state struct from stack to %edi
    mov 4(%esp), %edi

    # Loads stored cs register
    mov 40(%edi), %eax
    and $3, %eax
    jz __run_task_kernelspace
    jmp __run_task_userspace


__run_task_kernelspace:
    mov 8(%edi), %ebp
    mov 16(%edi), %esi
    mov 20(%edi), %edx
    mov 24(%edi), %ecx
    mov 28(%edi), %ebx
    mov 32(%edi), %eax

    mov 48(%edi), %esp
    push 44(%edi)
    push 40(%edi)
    push 36(%edi)

    mov 12(%edi), %edi

    iret

__run_task_userspace:
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

    push 52(%edi)
    push 48(%edi)
    push 44(%edi)
    push 40(%edi)
    push 36(%edi)

    mov 12(%edi), %edi

    iret

.size __run_task, . - __run_task
