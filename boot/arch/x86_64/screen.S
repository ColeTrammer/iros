.set VGA_BASE, 0xB8000
.set VGA_WIDTH, 80
.set VGA_HEIGHT, 25

.section .text
.global clear_screen
clear_screen:
    mov $VGA_BASE, %edi
    mov $(VGA_WIDTH * VGA_HEIGHT), %ecx
    mov $0x0720, %ax
    rep stosw
    ret
clear_character:
    movw $0x0720, (%eax, %ecx, 2)
    inc %ecx
    cmp $(VGA_WIDTH * VGA_HEIGHT), %ecx
    jge _clear_screen_return
    jmp clear_character
_clear_screen_return:
    ret

.global kprint
kprint:
    cmp $VGA_HEIGHT, (row_ptr)
    jl do_print
    movl $0, (row_ptr)
do_print:
    mov 4(%esp), %esi
    mov (row_ptr), %edi
    imul $(2 * VGA_WIDTH), %edi
    add $VGA_BASE, %edi
    mov $0, %ecx
print_char:
    movb (%esi, %ecx, 1), %dl
    cmp $0, %dl
    je _kprint_return
    movb $0x07, %dh
    movw %dx, (%edi, %ecx, 2)
    inc %ecx
    jmp print_char
_kprint_return:
    incl (row_ptr) 
    ret

.section .data
row_ptr:
    .long 0