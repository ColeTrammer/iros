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

    push %eax
    push 16(%esp)
    
    # FIXME: ensure the stack is 16 byte aligned
    call do_got_resolve
    mov %eax, %ebx

    pop %edx
    pop %ecx
    pop %eax

    add $8, %esp
    jmp *%ebx
.size got_resolver, . - got_resolver
