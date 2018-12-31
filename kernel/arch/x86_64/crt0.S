.section .text.startup
.global _start
_start:
    push %rdi
    push %rsi
    push %rdx
    call _init
    
    pop %rdx
    pop %rsi
    pop %rdi
    call kernel_main

    call _fini

.global __halt
__halt:
    cli
    hlt
    jmp __halt