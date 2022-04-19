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
    mov 28(%esp), %eax
    mov 32(%esp), %ebx
    mov 36(%esp), %ecx
    mov 40(%esp), %edx
    mov 44(%esp), %esi
    mov 48(%esp), %edi
    mov 52(%esp), %ebp
    
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
