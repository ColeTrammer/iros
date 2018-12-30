.section .text.startup
.global _start
_start:
    call _init

    call kernel_main

    call _fini

.global __halt
__halt:
    cli
    hlt
    jmp __halt