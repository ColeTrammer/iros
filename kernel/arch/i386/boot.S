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

    # Save eax and ebx (_init modifies them)
    push %eax
    push %ebx

    # Call the global constructors
    call _init

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
    jmp __halt

valid_boot_loader:
    # See if longmode is supported
    call supports_longmode
    test %eax, %eax
    jnz long_mode_yes

    # Else print error message and halt
    push $long_mode_no_str
    call kprint
    jmp __halt

long_mode_yes:
    mov $0x01, %eax
    cpuid
    test $(1 << 5), %edx
    jz msr_yes

    push $msr_no_str
    call kprint
    jmp __halt

msr_yes:
    # Output success string
    push $long_mode_yes_str
    call kprint
    
    # Set up paging structure
    mov $PDP, %eax
    or $0x03, %eax
    mov %eax, (PML4)

    mov $PD, %eax
    or $0x03, %eax
    mov %eax, (PDP)

    mov $PT, %eax
    or $0x03, %eax
    mov %eax, (PD)

    mov $PT, %eax
    mov $0x03, %edx
    mov $512, %ecx

add_page_table_entry:
    mov %edx, (%eax)
    add 0x1000, %edx
    add 0x08, %eax
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

    # Call C entry point
    call kernel_main

# Loop forever if the kernel returns
.global __halt
__halt:
    cli
    hlt
    jmp __halt

.section .data
.align 0x1000
PML4:
    .rept 512
        .quad 0
    .endr
PDP:
    .rept 512
        .quad 0
    .endr
PD:
    .rept 512
        .quad 0
    .endr
PT:
    .rept 512
        .quad 0
    .endr

.section .rodata
invalid_boot_loader_str:
.asciz "Invalid boot loader."
long_mode_yes_str:
.asciz "Long mode supported."
long_mode_no_str:
.asciz "Long mode unsupported."
msr_no_str:
.asciz "No model specific registers."