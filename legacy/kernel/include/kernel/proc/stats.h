#ifndef _KERNEL_PROC_STATS_H
#define _KERNEL_PROC_STATS_H 1

#include <stdint.h>

struct kmalloc_stats {
    uint64_t alloc_count;
    uint64_t free_count;
};

struct phys_page_stats {
    uint64_t phys_memory_allocated;
    uint64_t phys_memory_total;
    uint64_t phys_memory_max;
};

extern struct kmalloc_stats g_kmalloc_stats;
extern struct phys_page_stats g_phys_page_stats;

#endif /* _KERNEL_PROC_STATS_H */
