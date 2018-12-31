#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <kernel/mem/page.h>
#include <kernel/mem/page_frame_allocator.h>

static uint64_t page_bitmap[PAGE_BITMAP_SIZE / sizeof(uint64_t)];

static bool get_bit(uint64_t bit_index) {
    return page_bitmap[bit_index / (8 * sizeof(uint64_t))] & (1 << (bit_index % (8 * sizeof(uint64_t))));
}

static void set_bit(uint64_t bit_index, bool value) {
    if (value) {
        page_bitmap[bit_index / (8 * sizeof(uint64_t))] |= (1 << (bit_index % (8 * sizeof(uint64_t))));
    } else {
        page_bitmap[bit_index / (8 * sizeof(uint64_t))] &= ~(1 << (bit_index % (8 * sizeof(uint64_t))));
    }
}

static void mark_used(uint64_t phys_addr_start, uint64_t length) {
    uint64_t num_pages = length / PAGE_SIZE;
    uint64_t bit_index_base = phys_addr_start / PAGE_SIZE;
    for (uint64_t i = 0; i < num_pages; i++) {
        set_bit(bit_index_base + i, true);
    }
}

uint64_t get_next_phys_page() {
    for (uint64_t i = 0; i < PAGE_BITMAP_SIZE / sizeof(uint64_t); i++) {
        if (page_bitmap[i]) {
            uint64_t bit_index = i * 8 * sizeof(uint64_t);
            while (get_bit(bit_index)) { bit_index++; }
            set_bit(bit_index, true);
            return bit_index * PAGE_SIZE;
        }
    }
    return 0; // indicates there are no available physical pages
}

void free_phys_page(uint64_t phys_addr) {
    set_bit(phys_addr / PAGE_SIZE, false);
}

void init_page_frame_allocator(uint64_t kernel_phys_start, uint64_t kernel_phys_end, uint32_t *multiboot_info) {
    mark_used(0, 0x100000); // assume none of this area is available
    mark_used(kernel_phys_start, kernel_phys_end - kernel_phys_start);
    uint32_t *data = multiboot_info + 2;
    while (data < multiboot_info + multiboot_info[0] / sizeof(uint32_t)) {
        if (data[0] == 6) {
            uint64_t *mem = (uint64_t*) (data + 4);
            while ((uint32_t*) mem < data + data[1] / sizeof(uint32_t)) {
                if ((uint32_t) mem[2] == 2) {
                    mark_used(mem[0], mem[1]);
                }
                mem += data[2] / sizeof(uint64_t);
            }
        }
        data = (uint32_t*) ((uint64_t) data + data[1]);
        if ((uint64_t) data % 8 != 0) {
            data = (uint32_t*) (((uint64_t) data & ~0x7) + 8);
        }
    }
}