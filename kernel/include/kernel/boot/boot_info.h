#ifndef _KERNEL_BOOT_BOOT_INFO_H
#define _KERNEL_BOOT_BOOT_INFO_H 1

#define BOOT_INFO_MULTIBOOT2 1
#define BOOT_INFO_XEN        2
#ifndef __ASSEMBLER__
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct multiboot2_info;
struct xen_boot_info;

struct boot_info {
    int boot_info_type;
    uintptr_t initrd_phys_start;
    uintptr_t initrd_phys_end;
    const char *command_line;
    bool ide_use_default_ports;
    bool serial_debug;
    bool redirect_start_stdio_to_serial;
    bool poweroff_on_panic;
    void *memory_map;
    size_t memory_map_count;
};

void init_boot_info_from_multiboot2(const struct multiboot2_info *info);
void init_boot_info_from_xen(const struct xen_boot_info *info);
void init_boot_info(int boot_info_type, const void *info);

struct boot_info *boot_get_boot_info();
#endif

#endif /* _KERNEL_BOOT_BOOT_INFO_H */
