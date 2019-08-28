.section .text
.global main
main:
    mov $0x12345678, %rax
__halt:
    jmp __halt