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
.type _start, @function
_start:
    # Initialize the stack
    movl $stack_top, %esp
    
    # Call the global constructors
    call _init
    
    # Call C entry point
    call kernel_main

    # Loop forever if the kernel returns
    cli
1:  hlt
    jmp 1b

.size _start, . - _start