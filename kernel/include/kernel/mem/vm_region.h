#ifndef _KERNEL_MEM_VM_REGION_H
#define _KERNEL_MEM_VM_REGION_H

#include <stdint.h>
#include <stddef.h>

struct vm_region {
    uintptr_t start;
    uintptr_t end;

#define VM_WRITE (1UL << 1)
#define VM_USER (1UL << 2)
#define VM_GLOBAL (1UL << 8)
#define VM_NO_EXEC (1UL << 63)
    uint64_t flags;

#define VM_KERNEL_TEXT (1UL << 0)
#define VM_KERNEL_ROD (1UL << 1)
#define VM_KERNEL_DATA (1UL << 2)
#define VM_VGA (1UL << 3)
#define VM_INITRD (1UL << 4)
#define VM_KERNEL_HEAP (1UL << 5)
#define VM_KERNEL_STACK (1UL << 6)
#define VM_PROCESS_TEXT (1UL << 16)
#define VM_PROCESS_ROD (1UL << 17)
#define VM_PROCESS_DATA (1UL << 18)
#define VM_PROCESS_BSS (1UL << 19)
#define VM_PROCESS_HEAP (1UL << 20)
#define VM_PROCESS_STACK (1UL << 21)
    uint64_t type;

    struct vm_region *next;
};

struct vm_region *add_vm_region(struct vm_region *list, struct vm_region *to_add);
struct vm_region *remove_vm_region(struct vm_region *list, uint64_t type);
struct vm_region *get_vm_region(struct vm_region *list, uint64_t type);
int extend_vm_region_end(struct vm_region *list, uint64_t type, size_t num_pages);
int extend_vm_region_start(struct vm_region *list, uint64_t type, size_t num_pages);
int contract_vm_region_end(struct vm_region *list, uint64_t type, size_t num_pages);
int contract_vm_region_start(struct vm_region *list, uint64_t type, size_t num_pages);

#endif /* _KERNEL_MEM_VM_REGION_H */