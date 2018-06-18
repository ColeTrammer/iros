.section .text.startup
.global _start
.type _start, @function
_start: 
1:  hlt
    jmp 1b

.size _start, . - _start