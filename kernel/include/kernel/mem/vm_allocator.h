#ifndef _KERNEL_MEM_VM_ALLOCATOR_H
#define _KERNEL_MEM_VM_ALLOCATOR_H 1

#include <stddef.h>
#include <stdint.h>

void init_vm_allocator(uint64_t kernel_phys_start, uint64_t kernel_phys_end);

void *add_vm_pages(size_t n);
void remove_vm_pages(size_t n);

#endif /* _KERNEL_MEM_VM_ALLOCATOR_H */