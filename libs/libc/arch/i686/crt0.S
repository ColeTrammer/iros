.section .text.startup
.global _start
.type _start, @function
_start:
    # Load arguments into registers
    mov 4(%esp), %eax
    mov 8(%esp), %ebx
    mov 12(%esp), %esi
    mov 16(%esp), %edi

	# Set up end of the stack frame linked list.
	xor %ebp, %ebp
	push %ebp
	push %ebp
	mov %esp, %ebp

    # Align stack
    sub $16, %esp
    and $0xFFFFFFF0, %esp

    # Put initialize_standard_library() arguments onto stack
    mov %edi, 12(%esp)
    mov %esi, 8(%esp)
    mov %ebx, 4(%esp)
    mov %eax, 0(%esp)

    # Init libc
    call initialize_standard_library

    # Put main() arguments onto stack
    mov %edi, 8(%esp)
    mov %esi, 4(%esp)
    mov %ebx, 0(%esp)

    # Zero eax (main return value defaults to 0)
    xor %eax, %eax

    call main

    mov %eax, 0(%esp)
    call exit

.global __halt
__halt:
    jmp __halt

.size _start, . - _start
