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
.long MULTIBOOT_HEADER_START
.long _text_start
.long _data_end
.long _bss_end
ADDRESS_TAG_END:
.align 8
ENTRY_TAG_START:
.word 3
.word 1
.long ENTRY_TAG_END - ENTRY_TAG_START
.long start
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

# Declare relevant constants
.set MAGIC_CHECK, 0x36D76289

# Declare start for linker
.global start
.set start, _start

# Kernel entry point
.section .text
.code32
.global _start
_start:
    # Initialize the stack
    mov $stack_top, %esp

    # Save ebx and eax
    push %ebx
    push %eax

    # Clear screen
    call clear_screen32

    # Restore eax
    pop %eax

    # Check to make sure the boot loader is valid
    cmp $MAGIC_CHECK, %eax
    jz valid_boot_loader

    # Else print error message and hlt
    push $invalid_boot_loader_str
    call kprint32
    add $4, %esp
    jmp __halt

valid_boot_loader:    
    # See if longmode is supported
    call supports_longmode
    test %eax, %eax
    jnz long_mode_yes

    # Else print error message and halt
    push $long_mode_no_str
    call kprint32
    add $4, %esp
    jmp __halt

long_mode_yes:
    # Restore ebx
    pop %ebx

    # Output success string
    push $long_mode_yes_str
    call kprint32
    add $4, %esp
    
    # Set up paging structure by identity mapping the first page table
    # Or all values with 0x03 to mark present and r/w permissions

    # Write PDP0 to PML4
    mov $PDP0, %eax
    or $0x03, %eax
    mov %eax, (PML4)

    # Write PDP1 to PML4
    mov $PDP1, %eax
    or $0x03, %eax
    mov %eax, (PML4 + 510 * 8)

    # Write recursive mapping for PML4
    mov $PML4, %eax
    or $0x03, %eax
    mov %eax, (PML4 + 511 * 8)

    # Write PD0 to PDP0
    mov $PD0, %eax
    or $0x03, %eax
    mov %eax, (PDP0)

    # Write PD1 to PDP1
    mov $PD1, %eax
    or $0x03, %eax
    mov %eax, (PDP1)

    # Write PT0 to PD0
    mov $PT0, %eax
    or $0x03, %eax
    mov %eax, (PD0)

    # Write PT1 to PD0
    mov $PT1, %eax
    or $0x03, %eax
    mov %eax, (PD0 + 1 * 8)

    # Write PT2 to PD0
    mov $PT2, %eax
    or $0x03, %eax
    mov %eax, (PD0 + 2 * 8)    

    # Write PT3 to PD1
    mov $PT3, %eax
    or $0x03, %eax
    mov %eax, (PD1)

    # Set up for loop
    mov $PT0, %eax
    mov $0x03, %edx
    mov $512, %ecx

add_page_table_entry_0:
    # Writes identity mapping to throughout page table
    mov %edx, (%eax)
    add $0x1000, %edx
    add $0x08, %eax
    loop add_page_table_entry_0

    # Set up for loop
    mov $PT1, %eax
    mov $0x200003, %edx
    mov $512, %ecx

add_page_table_entry_1:
    # Writes identity mapping to throughout page table
    mov %edx, (%eax)
    add $0x1000, %edx
    add $0x08, %eax
    loop add_page_table_entry_1

    # Set up for loop
    mov $PT2, %eax
    mov $0x400003, %edx
    mov $512, %ecx

add_page_table_entry_2:
    # Writes identity mapping to throughout page table
    mov %edx, (%eax)
    add $0x1000, %edx
    add $0x08, %eax
    loop add_page_table_entry_2

    # Set up for loop
    mov $PT3, %eax
    mov $0x400003, %edx
    mov $512, %ecx
add_page_table_entry_3:
    # Writes identity mapping to throughout page table
    mov %edx, (%eax)
    add $0x1000, %edx
    add $0x08, %eax
    loop add_page_table_entry_3

enable_paging:
    # Enable cr4.PAE and cr4.PGE
    mov %cr4, %eax
    or $(1 << 5 | 1 << 7), %eax
    mov %eax, %cr4

    # Set EFER.LME and EFER.NXE
    mov $0xC0000080, %ecx
    rdmsr
    or $(1 << 8 | 1 << 11), %eax
    wrmsr

    # Set cr3
    mov $PML4, %eax
    mov %eax, %cr3

    # Set cr0.PE (WP is 1 << 16 but is undesireable)
    mov %cr0, %eax
    or $(1 << 31), %eax
    mov %eax, %cr0

    lgdtl (GDT_DESCRIPTOR)
    ljmp $0x08, $entry_64

# Loop forever
.global __halt
__halt:
    cli
    hlt
    jmp __halt

.code64
entry_64:
    # Segment registers
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    # Call _init
    call _init

    # Saves Multiboot2 Address
    push %rbx

    # Prepares kernel and returns jump address
    mov %rbx, %rdi
    call prepare_kernel_for_jump

    # Save kernel boot info address
    push %rax

    # Call this before jumping away
    call _fini

    # Restores kernel boot info address
    pop %rax

    # Writes Kernel Entry to rbx
    mov (%rax), %rbx

    # Writes Kernel Phys Start to rdi
    mov 8(%rax), %rdi

    # Writes Kernel Phys End to rsi
    mov 16(%rax), %rsi

    # Writes Initrd Phys Start to rdx
    mov 24(%rax), %rdx

    # Writes Initrd Phys End to rcx
    mov 32(%rax), %rcx

    # Writes Multiboot2 Address to r8
    movq 40(%rax), %r8

    # Jump to kernel entry
    jmp *%rbx

    # Loop forever
    jmp __halt

.section .bss
.align 0x1000
PML4:
    .skip 0x1000
PDP0:
    .skip 0x1000
PDP1:
    .skip 0x1000
PD0:
    .skip 0x1000
PD1:
    .skip 0x1000
PT0:
    .skip 0x1000
PT1:
    .skip 0x1000
PT2:
    .skip 0x1000
PT3:
    .skip 0x1000

# Reserve stack for initial thread
.section .bss
.align 16
stack_bottom:
.skip 16384
stack_top:

# Data for GTD
.section .data
GDT:
GDT_NULL:
    .short 0xFFFF
    .short 0
    .byte 0
    .byte 0
    .byte 0b00001111
    .byte 0
GDT_CODE:
    .short 0xFFFF
    .short 0
    .byte 0
    .byte 0b10011011
    .byte 0b00101111 # Change to 0b01001111 to keep 32 bit mode
    .byte 0
GDT_DATA:
    .short 0xFFFF
    .short 0
    .byte 0
    .byte 0b10010010
    .byte 0b00001111
    .byte 0
GDT_DESCRIPTOR:
    .short . - GDT - 1
    .long GDT
    .long 0

.section .rodata
invalid_boot_loader_str:
.asciz "Invalid boot loader."
long_mode_yes_str:
.asciz "Long mode supported."
long_mode_no_str:
.asciz "Long mode unsupported."
kernel_module_found_str:
.asciz "Found the kernel."
kernel_module_not_found_str:
.asciz "Kernel not found."