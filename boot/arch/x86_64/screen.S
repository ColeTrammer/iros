.set VGA_BASE, 0xB8000
.set VGA_WIDTH, 80
.set VGA_HEIGHT, 25

.section .text
.global clear_screen
clear_screen:
    mov $VGA_BASE, %eax
    mov $0, %ecx
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
    cmp $VGA_HEIGHT, (row)
    jl do_print
    movl $0, (row)
do_print:
    mov 4(%esp), %edx
    mov $VGA_BASE, %eax
    mov (row), %ecx
    imul $(2 * VGA_WIDTH), %ecx
    add %ecx, %eax
    mov $0, %ecx
print_char:
    mov (%edx, %ecx, 1), %edi
    and $0xFF, %edi
    cmp $0, %edi
    je _kprint_return
    or $0x0700, %edi
    mov %edi, (%eax, %ecx, 2)
    inc %ecx
    jmp print_char
_kprint_return:
    incl (row) 
    ret

.section .data
row:
    .long 0