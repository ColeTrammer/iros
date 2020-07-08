.section .text.ap_boot
.code16
.set TRAMPOLINE_PHYS_START, 0x8000
.global _trampoline_entry
_trampoline_entry:
    cli

    # Enable cr4.PAE and cr4.PGE and cr4.OSFXSR and cr4.OSXMMEEXCPT
    mov %cr4, %eax
    or $(1 << 5 | 1 << 7 | 1 << 9 | 1 << 10), %eax
    mov %eax, %cr4

    # Set EFER.LME and EFER.NXE
    mov $0xC0000080, %ecx
    rdmsr
    or $(1 << 8 | 1 << 11), %eax
    wrmsr

    # Set cr3
    mov (_cr3 - _trampoline_entry + TRAMPOLINE_PHYS_START), %eax
    mov %eax, %cr3


    # Set cr0.PE and cr0.WP and cr0.MP, cr0.NE, and cr0.PM; Clear cr0.EM
    mov %cr0, %eax
    or $(1 << 31 | 1 << 16 | 1 << 5 | 1 << 1 | 1 << 0), %eax
    and $(~(1 << 2)), %eax
    mov %eax, %cr0


    lgdtl (_GDT_DESCRIPTOR - _trampoline_entry + TRAMPOLINE_PHYS_START)
    ljmp $0x08, $(_trampoline_entry64 - _trampoline_entry + TRAMPOLINE_PHYS_START)

.code64
_trampoline_entry64:
    # Clear segment registers
    mov $0, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    # Use important values from the trampoline
    mov (_idt - _trampoline_entry + TRAMPOLINE_PHYS_START), %rax
    lidt (%rax)
    mov (_rsp - _trampoline_entry + TRAMPOLINE_PHYS_START), %rsp
    mov (_processor - _trampoline_entry + TRAMPOLINE_PHYS_START), %rdi

    # Jump to the kernel
    movabs $init_ap, %rax
    jmp *%rax

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
    .byte 0b10011011
    .byte 0b00101111
    .byte 0
_GDT_DATA:
    .short 0xFFFF
    .short 0
    .byte 0
    .byte 0b10010010
    .byte 0b00001111
    .byte 0
_GDT_DESCRIPTOR:
    .short . - _GDT - 1
    .long _GDT - _trampoline_entry + TRAMPOLINE_PHYS_START
    .long 0

_cr3:
.skip 8
_idt:
.skip 8
_rsp:
.skip 8
_processor:
.skip 8
