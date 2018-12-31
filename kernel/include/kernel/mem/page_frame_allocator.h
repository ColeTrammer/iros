#ifndef _KERNEL_MEM_PAGE_FRAME_ALLOCATOR_H
#define _KERNEL_MEM_PAGE_FRAME_ALLOCATOR_H 1

#include <stdint.h>

#include <kernel/mem/page.h>

#define PAGE_BITMAP_SIZE (PAGE_SIZE * 32)

void init_page_frame_allocator(uint64_t kernel_phys_start, uint64_t kernel_phys_end, uint32_t *multiboot_info);
uint64_t get_next_phys_page();
void free_phys_page(uint64_t phys_addr);

#endif /* _KERNEL_MEM_PAGE_FRAME_ALLOCATOR_H */