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
.long _kernel_entry32
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

.set VGA_BASE, 0xB8000
.set VGA_WIDTH, 80
.set VGA_HEIGHT, 25

.section .text.startup
.global _start32
_start32:
    # Load early stack
    mov $(early_stack_top - _start32 + _kernel_entry32), %esp

    # Save eax (should be equal to MULTIBOOT2_MAGIC_CHECK)
    mov %eax, %ebp
    
    # Save ebx (points to the multiboot2 info)
    push %ebx

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
    # Restore the multiboot2 info into edi
    pop %edi

    # The bootloader should have set things up properly, so
    # we can let the kernel handle everything in C.
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

# The data fields must be in the same section to allow relative offsets to be used.
_invalid_boot_loader_str:
.asciz "Invalid boot loader."

early_stack_bottom:
.skip 0x100
early_stack_top:

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
