.section .text.startup
.global _start
.type _start, @function
_start:
	# Set up end of the stack frame linked list.
	xor %rbp, %rbp
	pushq %rbp
	pushq %rbp
	movq %rsp, %rbp

    push %rcx    # ENVP
    push %rdx    # ARGV
    push %rsi    # ARGC

    callq initialize_standard_library

    pop %rdi     # ARGC
    pop %rsi     # ARGV
    pop %rdx     # ENVP

    xorq %rax, %rax # Zero RAX

    sub $8, %rsp # Alignment
    callq main

    movq %rax, %rdi
    callq exit

.global __halt
__halt:
    jmp __halt

.size _start, . - _start
