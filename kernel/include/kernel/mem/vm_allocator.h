#ifndef _KERNEL_MEM_VM_ALLOCATOR_H
#define _KERNEL_MEM_VM_ALLOCATOR_H 1

#include <stddef.h>
#include <stdint.h>

#include "vm_region.h"

void init_vm_allocator(uintptr_t initrd_phys_start, uintptr_t initrd_phys_end);

void *add_vm_pages(size_t n);
void remove_vm_pages(size_t n);

struct vm_region *find_vm_region(uint64_t type);

#endif /* _KERNEL_MEM_VM_ALLOCATOR_H */