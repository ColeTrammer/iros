#ifndef _KERNEL_MEM_PAGE_FRAME_ALLOCATOR_H
#define _KERNEL_MEM_PAGE_FRAME_ALLOCATOR_H 1

#include <stdint.h>

#include <kernel/mem/page.h>

#define PAGE_BITMAP_SIZE (PAGE_SIZE * 32)

struct process;

void init_page_frame_allocator(uint32_t *multiboot_info);
void mark_used(uintptr_t phys_addr_start, uintptr_t length);
uintptr_t get_next_phys_page(struct process *process);
void free_phys_page(uintptr_t phys_addr, struct process *process);

unsigned long get_total_phys_memory(void);
unsigned long get_max_phys_memory(void);

#endif /* _KERNEL_MEM_PAGE_FRAME_ALLOCATOR_H */
