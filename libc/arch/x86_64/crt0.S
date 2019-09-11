.section .text.startup
.global _start
.type _start, @function
_start:
    pushq %rdi
    pushq %rsi

    callq initialize_standard_library

    callq _init

    popq %rsi
    popq %rdi

    callq main
    pushq %rax

    callq _fini

    popq %rdi
    callq exit

__halt:
    jmpq __halt

.size _start, . - _start