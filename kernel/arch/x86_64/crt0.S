.section .text.startup
.global _start
_start:
    push %rdi
    call _init
    
    pop %rdi
    call kernel_main

    call _fini

.global __halt
__halt:
    cli
    hlt
    jmp __halt