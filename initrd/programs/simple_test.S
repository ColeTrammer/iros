.section .text
.global main
main:
    mov $0x12345678, %rbx
    int $0x80
    retq