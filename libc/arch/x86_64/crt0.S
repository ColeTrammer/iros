.section .text.startup
.global _start
.type _start, @function
_start:
    xorq %rbp, %rbp # Zero RBP

    movq (%rsp), %rdx # ENVP
    movq 8(%rsp), %rsi # ARGV
    movq 16(%rsp), %rdi # ARGC

    callq initialize_standard_library

    callq _init

    popq %rdx
    popq %rsi
    popq %rdi

    xorq %rax, %rax # Zero RAX

    callq main
    pushq %rax

    callq _fini

    popq %rdi
    callq exit

__halt:
    jmp __halt

.size _start, . - _start