#ifndef _KERNEL_MEM_VM_ALLOCATOR_H
#define _KERNEL_MEM_VM_ALLOCATOR_H 1

#include <sys/types.h>
#include <stddef.h>
#include <stdint.h>

#include <kernel/fs/file.h>
#include <kernel/mem/vm_region.h>

void init_vm_allocator(uintptr_t initrd_phys_start, uintptr_t initrd_phys_end);

void *add_vm_pages_end(size_t n, uint64_t type);
void *add_vm_pages_start(size_t n, uint64_t type);
void remove_vm_pages_end(size_t n, uint64_t type);
void remove_vm_pages_start(size_t n, uint64_t type);

void *map_file(off_t length, uint64_t flags);

struct vm_region *find_vm_region(uint64_t type);
struct vm_region *clone_process_vm();

#endif /* _KERNEL_MEM_VM_ALLOCATOR_H */