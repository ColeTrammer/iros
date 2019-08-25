#ifndef _KERNEL_ARCH_X86_64_MEM_PAGE_H
#define _KERNEL_ARCH_X86_64_MEM_PAGE_H 1

#include <stdint.h>

#include <kernel/mem/vm_region.h>

#define PAGE_SIZE 4096

#define PML4_BASE ((uint64_t*) 0xFFFFFFFFFFFFF000)
#define PDP_BASE ((uint64_t*) 0xFFFFFFFFFFE00000)
#define PD_BASE ((uint64_t*) 0xFFFFFFFFC0000000)
#define PT_BASE ((uint64_t*) 0xFFFFFF8000000000)

#define MAX_PML4_ENTRIES (PAGE_SIZE / sizeof(uint64_t))
#define MAX_PDP_ENTRIES (MAX_PML4_ENTRIES)
#define MAX_PD_ENTRIES (MAX_PML4_ENTRIES)
#define MAX_PT_ENTRIES (MAX_PML4_ENTRIES)

#define PAGE_STRUCTURE_FLAGS (0x01UL | VM_WRITE)

#endif /* _KERNEL_ARCH_X86_64_MEM_PAGE_H */