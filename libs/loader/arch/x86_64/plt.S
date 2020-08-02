.internal do_got_resolve
.type got_resolver, @function
.global got_resolver
.internal got_resolver
got_resolver:
    push %rax
    push %rcx
    push %rdx
    push %rsi
    push %rdi
    push %r8
    push %r9

    mov 56(%rsp), %rdi
    mov 64(%rsp), %rsi
    call do_got_resolve
    mov %rax, %r11

    pop %r9
    pop %r8
    pop %rdi
    pop %rsi
    pop %rdx
    pop %rcx
    pop %rax

    add $16, %rsp
    jmp *%r11
.size got_resolver, . - got_resolver
