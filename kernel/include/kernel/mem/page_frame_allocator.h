#ifndef _KERNEL_MEM_PAGE_FRAME_ALLOCATOR_H
#define _KERNEL_MEM_PAGE_FRAME_ALLOCATOR_H 1

#include <stdint.h>

#include <kernel/mem/page.h>

#define PAGE_BITMAP_SIZE (PAGE_SIZE * 32)

void init_page_frame_allocator(uintptr_t kernel_phys_start, uintptr_t kernel_phys_end, uintptr_t initrd_phys_start, uintptr_t initrd_phys_end, uint32_t *multiboot_info);
uintptr_t get_next_phys_page();
void free_phys_page(uintptr_t phys_addr);

#endif /* _KERNEL_MEM_PAGE_FRAME_ALLOCATOR_H */