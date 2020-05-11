.global VGA_BASE
.set VGA_BASE, 0xB8000
.set VGA_WIDTH, 80
.set VGA_HEIGHT, 25

.section .text
.code32
.global clear_screen32
clear_screen32:
    push %edi
    mov $VGA_BASE, %edi
    mov $(VGA_WIDTH * VGA_HEIGHT), %ecx
    mov $0x0720, %ax
    rep stosw
    pop %edi
    ret

.global kprint32
kprint32:
    push %esi
    push %edi
    cmp $VGA_HEIGHT, (row_ptr)
    jl do_print
    movl $0, (row_ptr)
do_print:
    mov 12(%esp), %esi
    mov (row_ptr), %edi
    imul $(2 * VGA_WIDTH), %edi
    add $VGA_BASE, %edi
    mov $0, %ecx
print_char:
    movb (%esi, %ecx, 1), %dl
    cmp $0, %dl
    je _kprint32_return
    movb $0x07, %dh
    movw %dx, (%edi, %ecx, 2)
    inc %ecx
    jmp print_char
_kprint32_return:
    incl (row_ptr)
    pop %edi
    pop %esi
    ret

.section .data
row_ptr:
    .long 0
