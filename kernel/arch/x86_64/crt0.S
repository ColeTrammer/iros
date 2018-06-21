.section .text
.global _start
_start:
    call _init

    call kernel_main