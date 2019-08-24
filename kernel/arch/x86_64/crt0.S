.section .text.startup
.global _start
_start:
    mov $_kernel_stack_start, %rsp
    mov $KERNEL_VMA_RIGHT_SHIFT_40, %rax
    shl $40, %rax
    add %rax, %rsp

    push %rdi
    push %rsi
    push %rdx
    push %rcx
    push %r8
    call _init
    
    pop %r8
    pop %rcx
    pop %rdx
    pop %rsi
    pop %rdi
    call kernel_main

    call _fini

.global __halt
__halt:
    cli
    hlt
    jmp __halt

.section .bss.kernel_stack
.skip 0x4000
.global KERNEL_VM_STACK_START
KERNEL_VM_STACK_START:
.section .bss.temp_page
.skip 0x1000
.global _temp_page
_temp_page: