#ifndef _KERNEL_ARCH_X86_64_MEM_PAGE_H
#define _KERNEL_ARCH_X86_64_MEM_PAGE_H 1

#include <stdint.h>

#include <kernel/mem/vm_region.h>

#define PAGE_SIZE 4096

#define PML4_BASE ((uint64_t *) 0xFFFFFFFFFFFFF000)
#define PDP_BASE  ((uint64_t *) 0xFFFFFFFFFFE00000)
#define PD_BASE   ((uint64_t *) 0xFFFFFFFFC0000000)
#define PT_BASE   ((uint64_t *) 0xFFFFFF8000000000)

#define MAX_PML4_ENTRIES (PAGE_SIZE / sizeof(uint64_t))
#define MAX_PDP_ENTRIES  (MAX_PML4_ENTRIES)
#define MAX_PD_ENTRIES   (MAX_PML4_ENTRIES)
#define MAX_PT_ENTRIES   (MAX_PML4_ENTRIES)

#define PAGE_STRUCTURE_FLAGS (0x01UL | VM_WRITE)

#define VIRT_ADDR(pml4_offset, pdp_offset, pd_offset, pt_offset)                                                              \
    ((((pml4_offset) &0x100UL) ? 0xFFFFUL << 48 : 0UL) | (((pml4_offset) &0x1FFUL) << 39) | (((pdp_offset) &0x1FFUL) << 30) | \
     (((pd_offset) &0x1FFUL) << 21) | (((pt_offset) &0x1FFUL) << 12))

extern void _temp_page();
#define TEMP_PAGE ((uint64_t *) &_temp_page)

extern spinlock_t temp_page_lock;

struct process;

struct virt_page_info {
    uint64_t pml4_index;
    uint64_t pdp_index;
    uint64_t pd_index;
    uint64_t pt_index;
    uintptr_t pml4_entry;
    uintptr_t pdp_entry;
    uintptr_t pd_entry;
    uintptr_t pt_entry;
};

void do_unmap_page(uintptr_t virt_addr, bool free_phys, bool free_phys_structure, struct process *process);

void map_page_info(struct virt_page_info *info);
struct virt_page_info *map_page_with_info(uintptr_t virt_addr, uint64_t flags, struct process *process);

void create_phys_id_map();

#endif /* _KERNEL_ARCH_X86_64_MEM_PAGE_H */