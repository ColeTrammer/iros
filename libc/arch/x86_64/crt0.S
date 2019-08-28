.section .text.startup
.global _start
.type _start, @function
_start: 
    jmp main

.size _start, . - _start