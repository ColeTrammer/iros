#ifndef _KERNEL_MEM_PHYS_PAGE_H
#define _KERNEL_MEM_PHYS_PAGE_H 1

#include <stdint.h>

struct phys_page {
    uintptr_t phys_addr;
    int ref_count;
};

struct phys_page *allocate_phys_page(void);
struct phys_page *bump_phys_page(struct phys_page *page);

void drop_phys_page(struct phys_page *page);

#endif /* _KERNEL_MEM_PHYS_PAGE_H */
