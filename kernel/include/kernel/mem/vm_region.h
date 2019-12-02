#ifndef _KERNEL_MEM_VM_REGION_H
#define _KERNEL_MEM_VM_REGION_H

#include <stddef.h>
#include <stdint.h>

struct inode;

struct vm_region {
    uintptr_t start;
    uintptr_t end;

#define VM_WRITE   (1UL << 1)
#define VM_USER    (1UL << 2)
#define VM_GLOBAL  (1UL << 8)
#define VM_NO_EXEC (1UL << 63)
    uint64_t flags;

/* Defines global vm_regions */
#define VM_KERNEL_PHYS_ID (0)
#define VM_KERNEL_TEXT    (1)
#define VM_KERNEL_ROD     (2)
#define VM_KERNEL_DATA    (3)
#define VM_INITRD         (4)
#define VM_KERNEL_HEAP    (5)

/* Defines per process vm_regions */
#define VM_KERNEL_STACK (6)
#define VM_PROCESS_TEXT (16)
#define VM_PROCESS_ROD  (17)
#define VM_PROCESS_DATA (18)
#define VM_PROCESS_BSS  (19)
#define VM_PROCESS_HEAP (20)
#define VM_TASK_STACK   (21)

/* Defines non unique process regions */
#define VM_PROCESS_FILE                           (40)
#define VM_DEVICE_MEMORY_MAP_DONT_FREE_PHYS_PAGES (41)
#define VM_PROCESS_ANON_MAPPING                   (42)
    uint64_t type;

    struct inode *backing_inode;

    struct vm_region *next;
};

struct vm_region *add_vm_region(struct vm_region *list, struct vm_region *to_add);
struct vm_region *remove_vm_region(struct vm_region *list, uint64_t type);
struct vm_region *get_vm_region(struct vm_region *list, uint64_t type);
struct vm_region *get_vm_last_region(struct vm_region *list, uint64_t type);
int extend_vm_region_end(struct vm_region *list, uint64_t type, size_t num_pages);
int extend_vm_region_start(struct vm_region *list, uint64_t type, size_t num_pages);
int contract_vm_region_end(struct vm_region *list, uint64_t type, size_t num_pages);
int contract_vm_region_start(struct vm_region *list, uint64_t type, size_t num_pages);

#endif /* _KERNEL_MEM_VM_REGION_H */