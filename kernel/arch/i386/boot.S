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
    # Output success string
    push $long_mode_yes_str
    call kprint

    # Call C entry point
    call kernel_main

# Loop forever if the kernel returns
.global __halt
__halt:
    cli
    hlt
    jmp __halt

.section .rodata
invalid_boot_loader_str:
.asciz "Invalid boot loader."
long_mode_yes_str:
.asciz "Long mode supported."
long_mode_no_str:
.asciz "Long mode unsupported."