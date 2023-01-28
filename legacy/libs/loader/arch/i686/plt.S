.internal do_got_resolve
.type got_resolver, @function
.global got_resolver
.internal got_resolver
got_resolver:
    push %eax
    push %ecx
    push %edx

    # Get the plt offset, and divide by 8 to make it a proper
    # index into the relocation table, instead of a byte offset.
    mov 16(%esp), %eax
    shr $3, %eax

    # Get the dynamic object pointer
    mov 12(%esp), %edx

    # Ensure stack is 16 byte aligned
    push %ebp
    mov %esp, %ebp
    sub $8, %esp
    and $0xFFFFFFF0, %esp

    # Write arguments
    mov %eax, 4(%esp)
    mov %edx, 0(%esp)

    call do_got_resolve
    mov %ebp, %esp
    pop %ebp

    # Since all registers must be preserved, store the
    # new pointer on the stack, and ret to it later.
    mov %eax, 16(%esp)

    pop %edx
    pop %ecx
    pop %eax

    # Set the stack to written return address.
    lea 4(%esp), %esp
    ret
.size got_resolver, . - got_resolver
