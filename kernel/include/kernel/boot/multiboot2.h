#ifndef _KERNEL_BOOT_MULTIBOOT2_H
#define _KERNEL_BOOT_MULTIBOOT2_H 1

#include <stdint.h>
#include <sys/param.h>

struct multiboot2_info {
    uint32_t total_size;
    uint32_t reserved;
    uint8_t tags[0];
} __attribute__((packed));

struct multiboot2_tag {
#define MULTIBOOT2_TAG_SENTINEL            0
#define MULTIBOOT2_TAG_BOOT_COMMAND_LINE   1
#define MULTIBOOT2_TAG_BOOT_LOADER_NAME    2
#define MULTIBOOT2_TAG_MODULE              3
#define MULTIBOOT2_TAG_BASIC_MEMORY_INFO   4
#define MULTIBOOT2_TAG_BIOS_BOOT_DEVICE    5
#define MULTIBOOT2_TAG_MEMORY_MAP          6
#define MULTIBOOT2_TAG_VBE_INFO            7
#define MULTIBOOT2_TAG_FRAME_BUFFER_INFO   8
#define MULTIBOOT2_TAG_ELF_SYMBOLS         9
#define MULTIBOOT2_TAG_APM_TABLE           10
#define MULTIBOOT2_TAG_OLD_RSDP            14
#define MULTIBOOT2_TAG_NEW_RSDP            15
#define MULTIBOOT2_TAG_NETWORK_INFO        16
#define MULTIBOOT2_TAG_IMAGE_PHYSICAL_BASE 21
    uint32_t type;
    uint32_t size;
} __attribute__((packed));

struct multiboot2_boot_command_line_tag {
    struct multiboot2_tag base;
    uint32_t size;
    char value[0];
} __attribute__((packed));

struct multiboot2_module_tag {
    struct multiboot2_tag base;
    uint32_t module_start;
    uint32_t module_end;
    char name[0];

} __attribute__((packed));

struct multiboot2_memory_map_entry {
    uint64_t base_address;
    uint64_t length;
#define MULTIBOOT2_MEMORY_MAP_AVAILABLE 1
#define MULTIBOOT2_MEMORY_MAP_RESERVED  2
#define MULTIBOOT2_MEMORY_MAP_ACPI      3
#define MULTIBOOT2_MEMORY_MAP_PRESERVE  4
#define MULTIBOOT2_MEMORY_MAP_DEFECTIVE 5
    uint32_t type;
    uint32_t reserved;
} __attribute__((packed));

struct multiboot2_memory_map_tag {
    struct multiboot2_tag base;
    uint32_t entry_size;
    uint32_t entry_version;
    uint8_t entries[0];
} __attribute__((packed));

#define multiboot2_for_each_tag(info, iter)                                                                    \
    for (struct multiboot2_tag *iter = (void *) (info)->tags;                                                  \
         iter->type != MULTIBOOT2_TAG_SENTINEL && (uintptr_t) iter < ((uintptr_t)(info)) + (info)->total_size; \
         iter = (void *) ALIGN_UP(((uintptr_t) iter) + iter->size, 8))

#define multiboot2_for_each_memory_map_entry(tag, iter)                                                                                \
    for (struct multiboot2_memory_map_entry *iter = (void *) (tag)->entries; (uintptr_t) iter < ((uintptr_t)(tag)) + (tag)->base.size; \
         iter = ((void *) (iter)) + (tag)->entry_size)

#define MULTIBOOT2_DECLARE_AND_CAST_TAG(name, tag, type) struct multiboot2_##type##_tag *name = (void *) (tag)

#endif /* _KERNEL_BOOT_MULTIBOOT2_H */
