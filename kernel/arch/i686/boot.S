#include <kernel/boot/xen.h>
#include <kernel/boot/boot_info.h>

# Multiboot Header information
.set MAGIC, 0xE85250D6
.set ARCHITECTURE, 0
.set HEADER_LENGTH, (MULTIBOOT_HEADER_END - MULTIBOOT_HEADER_START)
.set CHECKSUM, -(MAGIC + ARCHITECTURE + HEADER_LENGTH)

# Multiboot Header itself
.section .multiboot
.align 8
MULTIBOOT_HEADER_START:
.long MAGIC
.long ARCHITECTURE
.long HEADER_LENGTH
.long CHECKSUM
HEADER_TAGS_START:
.align 8
INFO_REQUEST_TAG_START:
.word 1
.word 0
.long INFO_REQUEST_TAG_END - INFO_REQUEST_TAG_START
.long 4 # Request basic mem info
.long 1 # Request boot command line
.long 3 # Request modules
.long 6 # Request mem map
INFO_REQUEST_TAG_END:
.align 8
ADDRESS_TAG_START:
.word 2
.word 0
.long ADDRESS_TAG_END - ADDRESS_TAG_START
.long _multiboot_header_start_boot
.long _text_start_boot
.long _data_end_boot
.long _bss_end_boot
ADDRESS_TAG_END:
.align 8
ENTRY_TAG_START:
.word 3
.word 1
.long ENTRY_TAG_END - ENTRY_TAG_START
.long _kernel_entry32 + (_start_multiboot2 - _start32)
ENTRY_TAG_END:
.align 8
MODULE_ALIGNMENT_TAG_START:
.word 6
.word 0
.long MODULE_ALIGNMENT_TAG_END - MODULE_ALIGNMENT_TAG_START
MODULE_ALIGNMENT_TAG_END:
.align 8
CLOSING_TAG_START:
.word 0
.word 0
.long CLOSING_TAG_END - CLOSING_TAG_START
CLOSING_TAG_END:
MULTIBOOT_HEADER_END:

.set MULTIBOOT2_MAGIC_CHECK, 0x36D76289

.section .note
.align 4
.long 4
.long XEN_ELFNOTE_PHYS32_ENTRY_DATA_END - XEN_ELFNOTE_PHYS32_ENTRY_DATA_START
.long XEN_ELFNOTE_PHYS32_ENTRY
.asciz "Xen"
XEN_ELFNOTE_PHYS32_ENTRY_DATA_START:
.long _kernel_entry32 + (_start_xen - _start32)
.align 4
XEN_ELFNOTE_PHYS32_ENTRY_DATA_END:

.set VGA_BASE, 0xB8000
.set VGA_WIDTH, 80
.set VGA_HEIGHT, 25

.section .text.startup
.global _start32
_start32:
    # Load early stack
    mov $(early_stack_top - _start32 + _kernel_entry32), %esp

    # Save boot info type and pointer
    push %eax
    push %ebx

    # Setup up the kernel page tables, so we can jump to
    # the higher-half kernel address.

    # Write PT_LOW to the PD
    mov $(_PT_LOW - _start32 + _kernel_entry32), %eax
    or $0x03, %eax
    mov %eax, (_PD - _start32 + _kernel_entry32)

    # Write PT_HIGH to the PD
    mov $(_PT_HIGH - _start32 + _kernel_entry32), %eax
    or $0x03, %eax
    mov %eax, (_PD - _start32 + _kernel_entry32 + 4 * 768)

    # Write an identity map from 0x000000 to 0x400000
    mov $(_PT_LOW - _start32 + _kernel_entry32), %eax
    mov $3, %edx
    mov $1024, %ecx

_add_page_table_low:
    mov %edx, (%eax)
    add $0x1000, %edx
    add $0x04, %eax
    loop _add_page_table_low

    # Write an identity map from 0x000000 to 0x400000
    mov $(_PT_HIGH - _start32 + _kernel_entry32), %eax
    mov $0x200003, %edx
    mov $1024, %ecx

_add_page_table_high:
    mov %edx, (%eax)
    add $0x1000, %edx
    add $0x04, %eax
    loop _add_page_table_high

    # Set cr3
    mov $(_PD - _start32 + _kernel_entry32), %eax
    mov %eax, %cr3

    # Enable Paging
    mov %cr0, %eax
    or $0x80000001, %eax
    mov %eax, %cr0

    # Restore boot info into %ebx
    pop %ebx
    pop %edx

    # Set eip to the new higher half address space
    lea higher_half, %ecx
    jmp *%ecx

higher_half:
    # Load higher half stack
    mov $BOOT_KERNEL_VM_STACK_START, %esp

    # Enable cr4.PGE and cr4.OSFXSR and cr4.OSXMMEEXCPT
    mov %cr4, %eax
    or $(1 << 7 | 1 << 9 | 1 << 10), %eax
    mov %eax, %cr4

    # Set cr0.PE and cr0.WP and cr0.MP and cr0.NE; Clear cr0.EM
    mov %cr0, %eax
    or $(1 << 31 | 1 << 16 | 1 << 5 | 1 << 1), %eax
    and $(~(1 << 2)), %eax
    mov %eax, %cr0

    # Now at last tranfer control to the kernel
    sub $16, %esp

    mov %ebx, 4(%esp)
    mov %edx, 0(%esp)
    call kernel_main

_l:
    jmp _l

# Print a null terminated string to VGA memory
_kprint32:
    mov $VGA_BASE, %esi
    mov $0, %ecx
_print_char:
    movb (%edi, %ecx, 1), %dl
    cmp $0, %dl
    je _kprint32_return
    movb $0x07, %dh
    movw %dx, (%esi, %ecx, 2)
    inc %ecx
    jmp _print_char
_kprint32_return:
    ret

.global __halt
__halt:
    cli
    hlt
    jmp __halt

.global _start_multiboot2
_start_multiboot2:
    # Save eax (should be equal to MULTIBOOT2_MAGIC_CHECK)
    mov %eax, %ebp

    # Save multiboot2 structure
    mov %ebx, %esi

    # Clear screen
    mov $VGA_BASE, %edi
    mov $(VGA_WIDTH * VGA_HEIGHT), %ecx
    mov $0x0720, %ax
    rep stosw

    # Check bootloader validity
    cmp $MULTIBOOT2_MAGIC_CHECK, %ebp
    je _valid_bootloader

    mov $(_invalid_boot_loader_str - _start32 + _kernel_entry32), %edi
    call _kprint32
    jmp _l

_valid_bootloader:
    # Restore boot info structure
    mov %esi, %ebx
    mov $BOOT_INFO_MULTIBOOT2, %eax
    jmp _start32

.global _start_xen
_start_xen:
    # Save xen info structure
    mov %ebx, %esi

    # Clear screen
    mov $VGA_BASE, %edi
    mov $(VGA_WIDTH * VGA_HEIGHT), %ecx
    mov $0x0720, %ax
    rep stosw

    # Restore boot info structure
    mov %esi, %ebx
    mov $BOOT_INFO_XEN, %eax
    jmp _start32

# The data fields must be in the same section to allow relative offsets to be used.
_invalid_boot_loader_str:
.asciz "Invalid boot loader."

early_stack_bottom:
.skip 0x100
early_stack_top:

.align 0x1000
_KERNEL_PAGE_TABLE_START:
_PD:
    .skip 0x1000
_PT_LOW:
    .skip 0x1000
_PT_HIGH:
    .skip 0x1000
_KERNEL_PAGE_TABLE_END:

.section .bss.kernel_stack
.skip 0x1000
BOOT_KERNEL_VM_STACK_START:
.section .bss.reserved_pages

# Dummy .init, .fini, .ctors, .dtors so that the toolchain thinks the kernel executable is valid
.align 16
.section .init
.skip 16
.section .fini
.skip 16
