#ifndef _KERNEL_BOOT_XEN_H
#define _KERNEL_BOOT_XEN_H 1

#define XEN_ELFNOTE_INFO           0
#define XEN_ELFNOTE_ENTRY          1
#define XEN_ELFNOTE_HYPERCALL_PAGE 2
#define XEN_ELFNOTE_VIRT_BASE      3
#define XEN_ELFNOTE_GUEST_OS       6
#define XEN_ELFNOTE_LOADER         8
#define XEN_ELFNOTE_PAE_MODE       9
#define XEN_ELFNOTE_PHYS32_ENTRY   18

#define XEN_BOOT_MAGIC 0x336EC578
#ifndef __ASSEMBLER__
struct xen_boot_info {
    uint32_t magic;
    uint32_t version;
    uint32_t flags;
    uint32_t module_count;
    uint64_t modules;
    uint64_t command_line;
    uint64_t rsdp;
    uint64_t memory_map;
    uint32_t memory_map_entry_count;
    uint32_t resserved;
} __attribute__((packed));

struct xen_module_entry {
    uint64_t addr;
    uint64_t size;
    uint64_t command_line;
    uint64_t reserved;
} __attribute__((packed));

struct xen_memory_map_entry {
    uint64_t base_address;
    uint64_t length;
    uint32_t type;
    uint32_t reserved;
} __attribute__((packed));
#endif

#endif /* _KERNEL_BOOT_XEN_H */
