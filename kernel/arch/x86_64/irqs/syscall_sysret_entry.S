#include <kernel/hal/x86_64/gdt.h>
#include <kernel/hal/x86_64/processor.h>

.section .text
.global syscall_sysret_entry
.type syscall_sysret_entry, @function
syscall_sysret_entry:
    # Save user space rsp
    mov %rsp, %gs:TSS_OFFSET + TSS_RSP1_OFFSET

    # Load the kernel rsp
    mov %gs:TSS_OFFSET + TSS_RSP0_OFFSET, %rsp

    # Fake an IRQ stack since system calls expect it.
    push $USER_DATA_SELECTOR              # ds
    push %gs:TSS_OFFSET + TSS_RSP1_OFFSET # rsp
    push %r11                             # rflags
    push $USER_CODE_SELECTOR              # cs
    push %rcx                             # return address

    push %rax
    push %rbx
    push %rcx
    push %rdx
    push %rsi
    push %rdi
    push %rbp
    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15

    mov %rsp, %rdi
    callq do_syscall

    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rbp
    pop %rdi
    pop %rsi
    pop %rdx
    pop %rcx
    pop %rbx
    pop %rax

    # We can't use sysret if the system call isn't meant to return (like for a successful execve).
    cmpq %rcx, (%rsp)
    jne use_iret

    # Check if %rcx is a cannonical address. This is necessary due to a bug in intel processors.
    shl $(64 - 48), %rcx
    sar $(64 - 48), %rcx
    cmpq %rcx, (%rsp)
    jne use_iret

    # Restore user space rsp 
    mov 24(%rsp), %rsp

    # Re-enable IRQs for userspace
    sti

    sysretq

use_iret:
    iretq

.size syscall_sysret_entry, . - syscall_sysret_entry
