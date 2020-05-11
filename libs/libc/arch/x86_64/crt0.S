.section .text.startup
.global _start
.type _start, @function
_start:
	# Set up end of the stack frame linked list.
	xor %rbp, %rbp
	pushq %rbp
	pushq %rbp
	movq %rsp, %rbp

    movq 16(%rsp), %rdx # ENVP
    movq 24(%rsp), %rsi # ARGV
    movq 32(%rsp), %rdi # ARGC

    callq initialize_standard_library

    movq 16(%rsp), %rdx # ENVP
    movq 24(%rsp), %rsi # ARGV
    movq 32(%rsp), %rdi # ARGC

    xorq %rax, %rax # Zero RAX

    callq main

    movq %rax, %rdi
    callq exit

.global __halt
__halt:
    jmp __halt

.size _start, . - _start
