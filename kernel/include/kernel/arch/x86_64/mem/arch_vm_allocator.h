#ifndef _KERNEL_ARCH_X86_64_MEM_ARCH_VM_ALLOCATOR_H
#define _KERNEL_ARCH_X86_64_MEM_ARCH_VM_ALLOCATOR_H 1

#include <stdint.h>

void *get_identity_phys_addr_mapping(uintptr_t phys_addr);

#endif /* _KERNEL_ARCH_X86_64_MEM_ARCH_VM_ALLOCATOR_H */
