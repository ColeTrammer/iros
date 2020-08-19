#ifndef _KERNEL_PROC_STATS_H
#define _KERNEL_PROC_STATS_H 1

#include <stdint.h>

struct kmalloc_stats {
    uint64_t alloc_count;
    uint64_t free_count;
};

extern struct kmalloc_stats g_kmalloc_stats;

#endif /* _KERNEL_PROC_STATS_H */
