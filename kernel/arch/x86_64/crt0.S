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
.code32
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
    # Check for CPUID availability by checking whether bit 22 of eflags is settable
    pushf
    pop %ecx
    mov %ecx, %edx
    xor $(1 << 21), %ecx
    push %ecx
    popf
    pushf
    pop %ecx
    cmp %edx, %ecx
    jnz _cpuid_yes

    mov $(_cpuid_no_str - _start32 + _kernel_entry32), %edi
    call _kprint32
    jmp _l

_cpuid_yes:
    # Use cpuid to check for extended functions
    mov $0x80000000, %eax
    cpuid
    cmp $0x80000001, %eax

    # Extension is available if eax >= 0x80000001
    jge _check_function_yes

    mov $(_check_function_no_str - _start32 + _kernel_entry32), %edi
    call _kprint32
    jmp _l

_check_function_yes:
    # Check for long mode
    mov $0x80000001, %eax
    cpuid
    test $(1 << 29), %edx
    jnz _longmode_yes

    mov $(_long_mode_no_str - _start32 + _kernel_entry32), %edi
    call _kprint32
    jmp _l

_longmode_yes:
    # Long mode is available, time to actually boot up.
    # Step 1 is to create a valid 64 bit page table.

    # Write PDP0 to the PML4
    mov $(_PDP0 - _start32 + _kernel_entry32), %eax
    or $0x03, %eax
    mov %eax, (_PML4 - _start32 + _kernel_entry32)

    # Write PDP1 to PML4
    mov $(_PDP1 - _start32 + _kernel_entry32), %eax
    or $0x03, %eax
    mov %eax, (_PML4 - _start32 + _kernel_entry32 + 510 * 8)

    # Write recursive mapping for PML4
    mov $(_PML4 - _start32 + _kernel_entry32), %eax
    or $0x03, %eax
    mov %eax, (_PML4 - _start32 + _kernel_entry32 + 511 * 8)

    # Write PD0 to PDP0
    mov $(_PD0 - _start32 + _kernel_entry32), %eax
    or $0x03, %eax
    mov %eax, (_PDP0 - _start32 + _kernel_entry32)

    # Write PD1 to PDP1
    mov $(_PD1 - _start32 + _kernel_entry32), %eax
    or $0x03, %eax
    mov %eax, (_PDP1 - _start32 + _kernel_entry32)

    # Write PT0 to PD0
    mov $(_PT0 - _start32 + _kernel_entry32), %eax
    or $0x03, %eax
    mov %eax, (_PD0 - _start32 + _kernel_entry32)

    # Write PT1 to PD0
    mov $(_PT1 - _start32 + _kernel_entry32), %eax
    or $0x03, %eax
    mov %eax, (_PD0 - _start32 + _kernel_entry32 + 1 * 8)

    # Write PT2 to PD1
    mov $(_PT2 - _start32 + _kernel_entry32), %eax
    or $0x03, %eax
    mov %eax, (_PD1 - _start32 + _kernel_entry32)

    mov $(_PT0 - _start32 + _kernel_entry32), %eax
    mov $0x03, %edx
    mov $512, %ecx

_add_page_table_entry_0:
    # Writes identity mapping to throughout page table 0 (0x000000 - 0x200000)
    mov %edx, (%eax)
    add $0x1000, %edx
    add $0x08, %eax
    loop _add_page_table_entry_0

    mov $(_PT1 - _start32 + _kernel_entry32), %eax
    mov $0x200003, %edx
    mov $512, %ecx

_add_page_table_entry_1:
    # Writes identity mapping to throughout page table 1 (0x200000 - 0x400000)
    mov %edx, (%eax)
    add $0x1000, %edx
    add $0x08, %eax
    loop _add_page_table_entry_1

    mov $(_PT2 - _start32 + _kernel_entry32), %eax
    mov $0x200003, %edx
    mov $512, %ecx

_add_page_table_entry_2:
    # Writes identity mapping to throughout page table 2 (upper address space)
    mov %edx, (%eax)
    add $0x1000, %edx
    add $0x08, %eax
    loop _add_page_table_entry_2

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
    mov $(_PML4 - _start32 + _kernel_entry32), %eax
    mov %eax, %cr3

    # Set cr0.PE and cr0.WP and cr0.MP and cr0.NE; Clear cr0.EM
    mov %cr0, %eax
    or $(1 << 31 | 1 << 16 | 1 << 5 | 1 << 1), %eax
    and $(~(1 << 2)), %eax
    mov %eax, %cr0

    # Restore the multiboot2 info into edi
    pop %edi

    lgdtl (_GDT_DESCRIPTOR - _start32 + _kernel_entry32)
    ljmp $0x08, $(_entry64 - _start32 + _kernel_entry32)

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

.code64
_entry64:
    # Make sure segment registers are at sane defaults
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    # Jump to the higher half kernel
    movabs $_kernel_start, %rax
    jmp *%rax

_kernel_start:
    movabs $KERNEL_VM_STACK_START, %rax
    movq %rax, %rsp

    push %rdi
    call _init
    pop %rdi

    call kernel_main

    call _fini

.global __halt
__halt:
    cli
    hlt
    jmp __halt

# The data fields must be in the same section to allow relative offsets to be used.
_invalid_boot_loader_str:
.asciz "Invalid boot loader."
_cpuid_no_str:
.asciz "Cpuid unsupported."
_check_function_no_str:
.asciz "No function to check long mode."
_long_mode_no_str:
.asciz "Long mode unsupported."

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
    .long _GDT - _start32 + _kernel_entry32
    .long 0

early_stack_bottom:
.skip 0x100
early_stack_top:

.align 0x1000
_PML4:
    .skip 0x1000
_PDP0:
    .skip 0x1000
_PDP1:
    .skip 0x1000
_PD0:
    .skip 0x1000
_PD1:
    .skip 0x1000
_PT0:
    .skip 0x1000
_PT1:
    .skip 0x1000
_PT2:
    .skip 0x1000

.section .bss.kernel_stack
.skip 0x4000
.global KERNEL_VM_STACK_START
KERNEL_VM_STACK_START:
.section .bss.reserved_pages
.global _temp_page
_temp_page:
.skip 0x1000
