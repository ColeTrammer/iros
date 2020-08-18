#ifndef _KERNEL_MEM_PHYS_PAGE_H
#define _KERNEL_MEM_PHYS_PAGE_H 1

#include <stdint.h>

#include <kernel/util/hash_map.h>
#include <kernel/util/list.h>

struct phys_page {
    struct list_node lru_list;
    struct hash_entry hash;
    uintptr_t phys_addr;
    int ref_count;
    off_t block_offset;
};

struct phys_page *allocate_phys_page(void);
struct phys_page *bump_phys_page(struct phys_page *page);

void drop_phys_page(struct phys_page *page);

#endif /* _KERNEL_MEM_PHYS_PAGE_H */
