# Multiboot Header information
.set ALIGN,   1 << 0           # flag to align modules on page boundaries
.set MEMINFO, 1 << 1           # flag to recieve memory map
.set FLAGS,   ALIGN | MEMINFO  # multiboot flags
.set MAGIC,   0x1BADB002       # magic number to verify this is a valid os
.set CHECK,   -(MAGIC + FLAGS) # proof that this is a valid os

# Multiboot Header itself
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECK

# Declare relevant constants
.set MAGIC_CHECK, 0x2BADB002

# Reserve stack for initial thread
.section .bss
.align 16
stack_bottom:
.skip 16384
stack_top:

.global start
.set start, _start

# Kernel entry point
.section .text
.global _start
_start:
    # Initialize the stack
    movl $stack_top, %esp

    # Save eax and ebx
    push %eax
    push %ebx

    # Clear screen
    call clear_screen

    # Restore those registers
    pop %ebx
    pop %eax

    # Check to make sure the boot loader is valid
    cmp $MAGIC_CHECK, %eax
    jz valid_boot_loader

    # Else print error message and hlt
    push $invalid_boot_loader_str
    call kprint
    add $4, %esp
    jmp __halt

valid_boot_loader:
    # See if longmode is supported
    call supports_longmode
    test %eax, %eax
    jnz long_mode_yes

    # Else print error message and halt
    push $long_mode_no_str
    call kprint
    add $4, %esp
    jmp __halt

long_mode_yes:
    # Output success string
    push $long_mode_yes_str
    call kprint
    add $4, %esp
    
    # Set up paging structure by identity mapping the first page table
    # Or all values with 0x03 to mark present and r/w permissions
    # Write PDP to PML3
    mov $PDP, %eax
    or $0x03, %eax
    mov %eax, (PML4)

    # Write PD to PDP
    mov $PD, %eax
    or $0x03, %eax
    mov %eax, (PDP)

    # Write PT to PD
    mov $PT, %eax
    or $0x03, %eax
    mov %eax, (PD)

    # Set up for loop
    and $0xFFFFF000, %eax
    mov $0x03, %edx
    mov $512, %ecx

add_page_table_entry:
    # Writes identity mapping to throughout page table
    mov %edx, (%eax)
    add $0x1000, %edx
    add $0x08, %eax
    loop add_page_table_entry

    # Enable cr4.PAE
    mov %cr4, %eax
    or $(1 << 5), %eax
    mov %eax, %cr4

    # Set EFER.LME
    mov $0xC0000080, %ecx
    rdmsr
    or $(1 << 8), %eax
    wrmsr

    # Set cr3
    mov $PML4, %eax
    mov %eax, %cr3

    # Set cr0.PE
    mov %cr0, %eax
    or $(1 << 31), %eax
    mov %eax, %cr0

    # Load GDT
    lgdtl (GDT_DESCRIPTOR)
    ljmp $0x8, $gdt_loaded 

gdt_loaded:
    # Call C entry point
    # call kernel_main

# Loop forever if the kernel returns
.global __halt
__halt:
    cli
    hlt
    jmp __halt

.section .bss
.align 0x1000
PML4:
    .skip 0x1000
PDP:
    .skip 0x1000
PD:
    .skip 0x1000
PT:
    .skip 0x1000

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
    .byte 0b01001111
    .byte 0
GDT_DATA:
    .short 0xFFFF
    .short 0
    .byte 0
    .byte 0b10010000
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