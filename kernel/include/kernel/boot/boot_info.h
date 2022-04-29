#ifndef _KERNEL_BOOT_BOOT_INFO_H
#define _KERNEL_BOOT_BOOT_INFO_H 1

#define BOOT_INFO_MULTIBOOT2 1
#define BOOT_INFO_XEN        2
#ifndef __ASSEMBLER__
#include <stdbool.h>
#include <stdint.h>

struct multiboot2_info;
struct multiboot2_memory_map_tag;
struct xen_info;

struct boot_info {
    uintptr_t initrd_phys_start;
    uintptr_t initrd_phys_end;
    const char *command_line;
    bool ide_use_default_ports;
    struct multiboot2_memory_map_tag *memory_map;
};

void init_boot_info_from_multiboot2(const struct multiboot2_info *info);
void init_boot_info_from_xen(const struct xen_info *info);

struct boot_info *boot_get_boot_info();
#endif

#endif /* _KERNEL_BOOT_BOOT_INFO_H */
