.section .text
.global _start
_start:
    mov $6, %edi
    mov $1, %esi
    mov $msg, %edx
    mov $len, %ecx
    int $0x80

    xor %esi, %esi
    mov $1, %edi
    int $0x80

.section .data
msg:
    .ascii "Hello, world!\n"
.set len, . - msg
