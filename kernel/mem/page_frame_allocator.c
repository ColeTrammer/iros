#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>

#include <kernel/hal/output.h>
#include <kernel/mem/page.h>
#include <kernel/mem/page_frame_allocator.h>
#include <kernel/proc/task.h>
#include <kernel/util/spinlock.h>

// #define PAGE_FRAME_ALLOCATOR_DEBUG

static uintptr_t page_bitmap[PAGE_BITMAP_SIZE / sizeof(uintptr_t)];
static spinlock_t bitmap_lock = SPINLOCK_INITIALIZER;

static bool get_bit(uintptr_t bit_index) {
    return page_bitmap[bit_index / (8 * sizeof(uintptr_t))] & (1UL << (bit_index % (8 * sizeof(uintptr_t))));
}

static void set_bit(uintptr_t bit_index, bool value) {
    if (value) {
        page_bitmap[bit_index / (8 * sizeof(uintptr_t))] |= (1UL << (bit_index % (8 * sizeof(uintptr_t))));
    } else {
        page_bitmap[bit_index / (8 * sizeof(uintptr_t))] &= ~(1UL << (bit_index % (8 * sizeof(uintptr_t))));
    }
}

void mark_used(uintptr_t phys_addr_start, uintptr_t length) {
    uintptr_t num_pages = NUM_PAGES(phys_addr_start, phys_addr_start + length);
    uintptr_t bit_index_base = phys_addr_start / PAGE_SIZE;
    for (uintptr_t i = 0; i < num_pages; i++) {
        set_bit(bit_index_base + i, true);
    }
}

uintptr_t get_next_phys_page(struct process *process) {
    spin_lock(&bitmap_lock);

    for (uintptr_t i = 0; i < PAGE_BITMAP_SIZE / sizeof(uintptr_t); i++) {
        if (~page_bitmap[i]) {
            uintptr_t bit_index = i * 8 * sizeof(uintptr_t);
            while (get_bit(bit_index)) {
                bit_index++;
            }
            set_bit(bit_index, true);

            spin_unlock(&bitmap_lock);

            process->resident_memory += PAGE_SIZE;
#ifdef PAGE_FRAME_ALLOCATOR_DEBUG
            debug_log("allocated: [ %#.16lX ]\n", bit_index * PAGE_SIZE);
#endif /* PAGE_FRAME_ALLOCATOR_DEBUG */
            return bit_index * PAGE_SIZE;
        }
    }

    spin_unlock(&bitmap_lock);
    return 0; // indicates there are no available physical pages
}

void free_phys_page(uintptr_t phys_addr, struct process *process) {
#ifdef PAGE_FRAME_ALLOCATOR_DEBUG
    debug_log("freed: [ %#.16lX ]\n", phys_addr);
#endif /* PAGE_FRAME_ALLOCATOR_DEBUG */

    spin_lock(&bitmap_lock);

    set_bit(phys_addr / PAGE_SIZE, false);
    if (process) {
        process->resident_memory -= PAGE_SIZE;
    }

    spin_unlock(&bitmap_lock);
}

static unsigned long phys_memory_total = 0;
static unsigned long phys_memory_max = 0;

unsigned long get_total_phys_memory(void) {
    return phys_memory_total;
}

unsigned long get_max_phys_memory(void) {
    return phys_memory_max;
}

void init_page_frame_allocator(uintptr_t kernel_phys_start, uintptr_t kernel_phys_end, uintptr_t initrd_phys_start,
                               uintptr_t initrd_phys_end, uint32_t *multiboot_info) {
    mark_used(0, 0x100000); // assume none of this area is available
    mark_used(kernel_phys_start, kernel_phys_end - kernel_phys_start);
    mark_used(initrd_phys_start, initrd_phys_end - initrd_phys_start);

    uint32_t *data = multiboot_info + 2;
    while (data < multiboot_info + multiboot_info[0] / sizeof(uint32_t)) {
        if (data[0] == 6) {
            uintptr_t *mem = (uintptr_t *) (data + 4);
            while ((uint32_t *) mem < data + data[1] / sizeof(uint32_t)) {
                if ((uint32_t) mem[2] == 2) {
                    mark_used(mem[0] & ~0xFFF, mem[1]);
                    phys_memory_total += mem[1] - (mem[0] & ~0xFFF);
                    phys_memory_max = MAX((mem[0] & ~0xFFF) + mem[1], phys_memory_max);
                }
                mem += data[2] / sizeof(uintptr_t);
            }
        }
        data = (uint32_t *) ((uintptr_t) data + data[1]);
        if ((uintptr_t) data % 8 != 0) {
            data = (uint32_t *) (((uintptr_t) data & ~0x7) + 8);
        }
    }

    debug_log("Finished Initializing Page Frame Allocator\n");
    debug_log("Max phys memory: [ %#lX ]\n", phys_memory_max);
}