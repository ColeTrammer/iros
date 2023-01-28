#include <stdint.h>

#include <kernel/boot/boot_info.h>
#include <kernel/hal/output.h>
#include <kernel/hal/x86/drivers/serial.h>
#include <kernel/util/spinlock.h>

extern spinlock_t debug_lock;

bool kprint(const char *str, size_t len) {
    if (boot_get_boot_info()->serial_debug) {
        return serial_write_message(str, len);
    }
    return true;
}

void dump_registers_to_screen() {
    uint32_t eax, ebx, ecx, edx, edi, esi, ebp, esp, cr3;
    asm("mov %%eax, %0" : "=m"(eax));
    asm("mov %%ebx, %0" : "=m"(ebx));
    asm("mov %%ecx, %0" : "=m"(ecx));
    asm("mov %%edx, %0" : "=m"(edx));
    asm("mov %%edi, %0" : "=m"(edi));
    asm("mov %%esi, %0" : "=m"(esi));
    asm("mov %%ebp, %0" : "=m"(ebp));
    asm("mov %%esp, %0" : "=m"(esp));
    asm("mov %%cr3, %%edx\n"
        "mov %%edx, %0"
        : "=m"(cr3)
        :
        : "edx");

    spin_lock_internal(&debug_lock, __func__, false);

#ifndef KERNEL_NO_DEBUG_COLORS
    printf("\n\33[31m");
#endif /* KERNEL_NO_DEBUG_COLORS */

    printf("EAX=%#.8X EBX=%#.8X\n", eax, ebx);
    printf("ECX=%#.8X EDX=%#.8X\n", ecx, edx);
    printf("EDI=%#.8X ESI=%#.8X\n", esi, edi);
    printf("EBP=%#.8X ESP=%#.8X\n", ebp, esp);

#ifndef KERNEL_NO_DEBUG_COLORS
    printf("CR3=%#.8X\033[0m\n", cr3);
#endif /* KERNEL_NO_DEBUG_COLORS */

    spin_unlock(&debug_lock);
}
