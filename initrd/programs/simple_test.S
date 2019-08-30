.section .text
.global main
main:
    mov $0x12345678, %rax
    int $0x80
    retq