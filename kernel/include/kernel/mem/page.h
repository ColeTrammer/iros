#ifndef _KERNEL_MEM_PAGE_H
#define _KERNEL_MEM_PAGE_H 1

#include <stdint.h>

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

void map_page(uint64_t virt_addr);
void unmap_page(uint64_t virt_addr);

#endif /* _KERNEL_MEM_PAGE_H */