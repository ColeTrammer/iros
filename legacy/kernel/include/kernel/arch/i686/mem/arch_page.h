#ifndef _KERNEL_ARCH_I686_MEM_ARCH_PAGE_H
#define _KERNEL_ARCH_I686_MEM_ARCH_PAGE_H 1

#include <stdint.h>

#include <kernel/mem/vm_region.h>

#define PAGE_SIZE 4096

#define MAX_PD_ENTRIES 1024
#define MAX_PT_ENTRIES 1024

#define KERNEL_HEAP_START 0xC8000000UL

struct process;

void do_map_phys_page(uintptr_t phys_addr, uintptr_t virt_addr, uint64_t flags, bool broadcast_flush_tlb, struct process *process);
void do_unmap_page(uintptr_t virt_addr, bool free_phys, bool free_phys_structure, bool broadcast_tlb_flush, struct process *process);

#endif /* _KERNEL_ARCH_I686_MEM_ARCH_PAGE_H */
