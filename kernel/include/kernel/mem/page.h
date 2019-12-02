#ifndef _KERNEL_MEM_PAGE_H
#define _KERNEL_MEM_PAGE_H 1

#include <stdbool.h>
#include <stdint.h>

#include <kernel/mem/vm_region.h>

// clang-format off
#include <kernel/arch/arch.h>
#include ARCH_SPECIFIC(mem/page.h)
// clang-format on

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

#define NUM_PAGES(start, end) ((((end) & ~0xFFF) - ((start) & ~0xFFF) + ((end) % PAGE_SIZE != 0 ? PAGE_SIZE : 0)) / PAGE_SIZE)

void clear_initial_page_mappings();

uintptr_t get_current_paging_structure();
uintptr_t create_paging_structure(struct vm_region *list, bool deep_copy);
uintptr_t clone_process_paging_structure();
void load_paging_structure(uintptr_t phys_addr);
void soft_remove_paging_structure(struct vm_region *list);
void remove_paging_structure(uintptr_t phys_addr, struct vm_region *list);

void map_vm_region_flags(struct vm_region *region);
void map_vm_region(struct vm_region *region);

void map_page(uintptr_t virt_addr, uint64_t flags);
void map_phys_page(uintptr_t phys_addr, uintptr_t virt_addr, uint64_t flags);
void unmap_page(uintptr_t virt_addr);

uintptr_t get_phys_addr(uintptr_t virt_addr);

#endif /* _KERNEL_MEM_PAGE_H */