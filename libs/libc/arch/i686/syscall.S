.section .text
.global __do_syscall
.type __do_syscall, @function
__do_syscall:
    // Save registers
    push %ebx
    push %ecx
    push %edx
    push %esi
    push %edi
    push %ebp

    // Load the arguments
    mov 24(%esp), %eax
    mov 28(%esp), %ebx
    mov 32(%esp), %ecx
    mov 46(%esp), %edx
    mov 40(%esp), %esi
    mov 44(%esp), %edi
    mov 48(%esp), %ebp
    
    // Do system call
    int $0x80

    // Restore registers
    pop %ebp
    pop %edi
    pop %esi
    pop %edx
    pop %ecx
    pop %ebx

    ret
.size __do_syscall, . - __do_syscall
