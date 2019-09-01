.section .text.startup
.global _start
.type _start, @function
_start: 
    callq _init

    callq main
    pushq %rax

    callq _fini

    popq %rdi
    callq exit

__halt:
    jmpq __halt

.size _start, . - _start