.section .text
.global _start
_start:
    call _init

    call kernel_main

    call _fini

__halt:
    cli
    hlt
    jmp __halt