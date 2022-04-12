#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include <kernel/boot/boot_info.h>
#include <kernel/boot/multiboot2.h>
#include <kernel/hal/block.h>
#include <kernel/hal/hal.h>
#include <kernel/hal/output.h>
#include <kernel/mem/kernel_vm.h>
#include <kernel/mem/page.h>
#include <kernel/mem/page_frame_allocator.h>
#include <kernel/proc/stats.h>
#include <kernel/proc/task.h>
#include <kernel/util/bitset.h>
#include <kernel/util/spinlock.h>

// #define PAGE_FRAME_ALLOCATOR_DEBUG

static uintptr_t page_bitset_storage[PAGE_BITMAP_SIZE / sizeof(uintptr_t)];
static struct bitset page_bitset;
static spinlock_t bitmap_lock = SPINLOCK_INITIALIZER;

struct phys_page_stats g_phys_page_stats = { 0 };

void mark_used(uintptr_t phys_addr_start, uintptr_t length) {
    uintptr_t num_pages = NUM_PAGES(phys_addr_start, phys_addr_start + length);
    uintptr_t bit_index_base = phys_addr_start / PAGE_SIZE;
    bitset_set_bit_sequence(&page_bitset, bit_index_base, num_pages);
}

void mark_available(uintptr_t phys_addr_start, uintptr_t length) {
    uintptr_t num_pages = NUM_PAGES(phys_addr_start, phys_addr_start + length);
    uintptr_t bit_index_base = phys_addr_start / PAGE_SIZE;
    bitset_clear_bit_sequence(&page_bitset, bit_index_base, num_pages);
}

static uintptr_t try_get_next_phys_page(struct process *process) {
    spin_lock(&bitmap_lock);

    size_t page_index;
    if (bitset_find_first_free_bit(&page_bitset, &page_index) == 0) {
        bitset_set_bit(&page_bitset, page_index);
        g_phys_page_stats.phys_memory_allocated += PAGE_SIZE;
        spin_unlock(&bitmap_lock);

        process->resident_memory += PAGE_SIZE;
#ifdef PAGE_FRAME_ALLOCATOR_DEBUG
        debug_log("allocated: [ %#.16lX ]\n", bit_index * PAGE_SIZE);
#endif /* PAGE_FRAME_ALLOCATOR_DEBUG */
        return page_index * PAGE_SIZE;
    }

    spin_unlock(&bitmap_lock);
    return 0;
}

uintptr_t get_next_phys_page(struct process *process) {
    uintptr_t try_1 = try_get_next_phys_page(process);
    if (try_1) {
        return try_1;
    }

    // The block cache will hopefully have unused pages it can release.
    block_trim_cache();

    uintptr_t try_2 = try_get_next_phys_page(process);
    if (try_2) {
        return try_2;
    }

    debug_log("Out of Physical Memory\n");
    abort();
    return 0;
}

uintptr_t get_contiguous_pages(size_t pages) {
    uintptr_t ret = 0;
    spin_lock(&bitmap_lock);

    size_t bit_start;
    if (bitset_find_first_free_bit_sequence(&page_bitset, pages, &bit_start) == 0) {
        bitset_set_bit_sequence(&page_bitset, bit_start, pages);
        ret = bit_start * PAGE_SIZE;
        g_phys_page_stats.phys_memory_allocated += pages * PAGE_SIZE;
    }

    spin_unlock(&bitmap_lock);
    return ret;
}

void free_phys_page(uintptr_t phys_addr, struct process *process) {
#ifdef PAGE_FRAME_ALLOCATOR_DEBUG
    debug_log("freed: [ %#.16lX ]\n", phys_addr);
#endif /* PAGE_FRAME_ALLOCATOR_DEBUG */

    spin_lock(&bitmap_lock);

    bitset_clear_bit(&page_bitset, phys_addr / PAGE_SIZE);
    if (process && process->resident_memory >= PAGE_SIZE) {
        process->resident_memory -= PAGE_SIZE;
    }

    g_phys_page_stats.phys_memory_allocated -= PAGE_SIZE;
    spin_unlock(&bitmap_lock);
}

void init_page_frame_allocator() {
    // Everything starts off allocated (reserved). Only usable segments (according to the bootloader) are made available.
    memset(page_bitset_storage, 0xFF, sizeof(page_bitset_storage));
    init_bitset(&page_bitset, page_bitset_storage, sizeof(page_bitset_storage), sizeof(page_bitset_storage) * CHAR_BIT);

    struct boot_info *boot_info = boot_get_boot_info();
    struct multiboot2_memory_map_tag *memory_map_tag = boot_info->memory_map;
    multiboot2_for_each_memory_map_entry(memory_map_tag, entry) {
        debug_log("Physical memory range: [ %#.16" PRIX64 ", %#.16" PRIX64 ", %u ]\n", entry->base_address & ~0xFFF, entry->length,
                  entry->type);
        if (entry->type == MULTIBOOT2_MEMORY_MAP_AVAILABLE) {
            // Memory below 0x100000 is reserved, and accounted as allocated.
            if (entry->base_address < 0x100000) {
                g_phys_page_stats.phys_memory_allocated += ALIGN_UP(
                    MIN(ALIGN_UP((entry->base_address & ~0xFFF) + entry->length, PAGE_SIZE), 0x100000) - (entry->base_address & ~0xFFF),
                    PAGE_SIZE);
            }

            mark_available(entry->base_address & ~0xFFF, entry->length);
            g_phys_page_stats.phys_memory_total += entry->length - (entry->base_address & ~0xFFF);
        }
        g_phys_page_stats.phys_memory_max =
            MAX(ALIGN_UP((entry->base_address & ~0xFFF) + entry->length, PAGE_SIZE), g_phys_page_stats.phys_memory_max);
    }

    mark_used(0, 0x100000); // assume none of this area is available for general purpose allocations, as device drivers might need it.
    g_phys_page_stats.phys_memory_allocated += ALIGN_UP(KERNEL_PHYS_END - KERNEL_PHYS_START, PAGE_SIZE);
    mark_used(KERNEL_PHYS_START, KERNEL_PHYS_END - KERNEL_PHYS_START);
    g_phys_page_stats.phys_memory_allocated += ALIGN_UP(boot_info->initrd_phys_end - boot_info->initrd_phys_start, PAGE_SIZE);
    mark_used(boot_info->initrd_phys_start, boot_info->initrd_phys_end - boot_info->initrd_phys_start);

    debug_log("Max phys memory: [ %#" PRIX64 " ]\n", g_phys_page_stats.phys_memory_max);
    debug_log("Total available memory: [ %#" PRIX64 " ]\n", g_phys_page_stats.phys_memory_total);
    debug_log("Kernel physical memory: [ %p, %p, %lu ]\n", (void *) KERNEL_PHYS_START, (void *) KERNEL_PHYS_END,
              (size_t) (KERNEL_PHYS_END - KERNEL_PHYS_START));
    debug_log("Initrd physical memory: [ %p, %p, %lu ]\n", (void *) boot_info->initrd_phys_start, (void *) boot_info->initrd_phys_end,
              (size_t) (boot_info->initrd_phys_end - boot_info->initrd_phys_start));
}
