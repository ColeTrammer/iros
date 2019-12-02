#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <kernel/hal/output.h>
#include <kernel/proc/task.h>
#include <kernel/util/spinlock.h>

#include <kernel/hal/x86_64/drivers/serial.h>
#include <kernel/hal/x86_64/drivers/vga.h>

static spinlock_t debug_lock = SPINLOCK_INITIALIZER;

bool kprint(const char *str, size_t len) {
    return serial_write_message(str, len);
}

int debug_log_internal(const char *func, const char *format, ...) {
    va_list parameters;
    va_start(parameters, format);

    spin_lock(&debug_lock);

#ifndef KERNEL_NO_DEBUG_COLORS
    int written = 0;
    if (get_current_task() == NULL || get_current_task()->process->pid == 1) {
        written += printf("\033[35mKernel  \033[37m(\033[34m %d \033[37m): ", 1);
    } else {
        printf("\033[32m%s \033[37m(\033[34m %d \033[37m): ", "Process", get_current_task()->process->pid);
    }
    written = printf("\033[36m%s\033[37m: ", func);
    written += vprintf(format, parameters);
#else
    int written = 0;
    if (get_current_task() == NULL || get_current_task()->process->pid == 1) {
        written += printf("Kernel  ( %d ): ", 1);
    } else {
        printf("%s ( %d ): ", "Process", get_current_task()->process->pid);
    }
    written = printf("%s: ", func);
    written += vprintf(format, parameters);
#endif /* KERNEL_NO_DEBUG_COLORS */

    spin_unlock(&debug_lock);

    va_end(parameters);
    return written;
}

void debug_log_assertion(const char *msg, const char *file, int line, const char *func) {
    disable_interrupts();

#ifndef KERNEL_NO_DEBUG_COLORS
    printf("\n\033[31m");
#endif /* KERNEL_NO_DEBUG_COLORS */

    printf("( %d ): Assertion failed: %s in %s at %s, line %d", get_current_task()->process->pid, msg, func, file, line);

#ifndef KERNEL_NO_DEBUG_COLORS
    printf("\033[0m\n");
#endif /* KERNEL_NO_DEBUG_COLORS */

    abort();
}

void dump_registers_to_screen() {
    uint64_t rax, rbx, rcx, rdx, rbp, rsp, rsi, rdi, r8, r9, r10, r11, r12, r13, r14, r15, cr3;
    asm("mov %%rax, %0" : "=m"(rax));
    asm("mov %%rbx, %0" : "=m"(rbx));
    asm("mov %%rcx, %0" : "=m"(rcx));
    asm("mov %%rdx, %0" : "=m"(rdx));
    asm("mov %%rbp, %0" : "=m"(rbp));
    asm("mov %%rsp, %0" : "=m"(rsp));
    asm("mov %%rsi, %0" : "=m"(rsi));
    asm("mov %%rdi, %0" : "=m"(rdi));
    asm("mov %%r8 , %0" : "=m"(r8));
    asm("mov %%r9 , %0" : "=m"(r9));
    asm("mov %%r10, %0" : "=m"(r10));
    asm("mov %%r11, %0" : "=m"(r11));
    asm("mov %%r12, %0" : "=m"(r12));
    asm("mov %%r13, %0" : "=m"(r13));
    asm("mov %%r14, %0" : "=m"(r14));
    asm("mov %%r15, %0" : "=m"(r15));
    asm("mov %%cr3, %%rdx\n"
        "mov %%rdx, %0"
        : "=m"(cr3)
        :
        : "rdx");

#ifndef KERNEL_NO_DEBUG_COLORS
    printf("\n\33[31m");
#endif /* KERNEL_NO_DEBUG_COLORS */

    printf("RAX=%#.16lX RBX=%#.16lX\n", rax, rbx);
    printf("RCX=%#.16lX RDX=%#.16lX\n", rcx, rdx);
    printf("RBP=%#.16lX RSP=%#.16lX\n", rbp, rsp);
    printf("RSI=%#.16lX RDI=%#.16lX\n", rsi, rdi);
    printf("R8 =%#.16lX R9 =%#.16lX\n", r8, r9);
    printf("R10=%#.16lX R11=%#.16lX\n", r10, r11);
    printf("R12=%#.16lX R13=%#.16lX\n", r12, r13);
    printf("R14=%#.16lX R15=%#.16lX\n", r14, r15);

#ifndef KERNEL_NO_DEBUG_COLORS
    printf("CR3=%#.16lX\033[0m\n", cr3);
#endif /* KERNEL_NO_DEBUG_COLORS */
}