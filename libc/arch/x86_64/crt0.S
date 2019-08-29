.section .text.startup
.global _start
.type _start, @function
_start: 
    callq _init

    callq main
_1:
    jmp _1

.size _start, . - _start