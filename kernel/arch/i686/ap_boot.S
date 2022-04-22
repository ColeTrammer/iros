.section .text.ap_boot
.code16
.set TRAMPOLINE_PHYS_START, 0x8000
.global _trampoline_entry
_trampoline_entry:
    cli

    # Enable cr4.PGE and cr4.OSFXSR and cr4.OSXMMEEXCPT
    mov %cr4, %eax
    or $(1 << 7 | 1 << 9 | 1 << 10), %eax
    mov %eax, %cr4

    # Set cr3
    mov (_cr3 - _trampoline_entry + TRAMPOLINE_PHYS_START), %eax
    mov %eax, %cr3

    # Set cr0.PE and cr0.WP and cr0.MP, cr0.NE, and cr0.PM; Clear cr0.EM
    mov %cr0, %eax
    or $(1 << 31 | 1 << 16 | 1 << 5 | 1 << 1 | 1 << 0), %eax
    and $(~(1 << 2)), %eax
    mov %eax, %cr0

    lgdt (_GDT_DESCRIPTOR - _trampoline_entry + TRAMPOLINE_PHYS_START)
    ljmp $0x08, $(_entry32 - _trampoline_entry + TRAMPOLINE_PHYS_START)

.code32
_entry32:
    # Set segment registers
    mov $16, %eax
    mov %eax, %ds
    mov %eax, %es
    mov %eax, %fs
    mov %eax, %gs
    mov %eax, %ss

    # Use important values from the trampoline
    mov (_idt - _trampoline_entry + TRAMPOLINE_PHYS_START), %eax
    lidt (%eax)
    mov (_esp - _trampoline_entry + TRAMPOLINE_PHYS_START), %esp

    sub $4, %esp
    and $0xFFFFFFF0, %esp

    mov (_processor - _trampoline_entry + TRAMPOLINE_PHYS_START), %eax
    mov %eax, 0(%esp)

    # Jump to the kernel
    mov $init_ap, %eax
    call %eax

_GDT:
_GDT_NULL:
    .short 0xFFFF
    .short 0
    .byte 0
    .byte 0
    .byte 0b00001111
    .byte 0
_GDT_CODE:
    .short 0xFFFF
    .short 0
    .byte 0
    .byte 0b10011010
    .byte 0b11001111
    .byte 0
_GDT_DATA:
    .short 0xFFFF
    .short 0
    .byte 0
    .byte 0b10010010
    .byte 0b11001111
    .byte 0
_GDT_DESCRIPTOR:
    .short . - _GDT - 1
    .long _GDT - _trampoline_entry + TRAMPOLINE_PHYS_START
    .long 0

_cr3:
.skip 4
_idt:
.skip 4
_esp:
.skip 4
_processor:
.skip 4
