.section .text.startup
.global _start
.type _start, @function
_start: 
    callq _init

    callq main

    callq _fini
_1:
    jmp _1

.size _start, . - _start