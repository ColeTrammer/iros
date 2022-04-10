.section .text.startup
.global _start
.type _start, @function
_start:
	# Set up end of the stack frame linked list.
	xor %ebp, %ebp
	push %ebp
	push %ebp
	mov %esp, %ebp

    push %ecx    # ENVP
    push %ebx    # ARGV
    push %esi    # ARGC

    # Note: does initialize_standard_library clobber its arguments? It probably does
    call initialize_standard_library

    xor %eax, %eax # Zero EAX

    # FIXME: ensure the stack is aligned
    call main

    push %eax
    call exit

.global __halt
__halt:
    jmp __halt

.size _start, . - _start
