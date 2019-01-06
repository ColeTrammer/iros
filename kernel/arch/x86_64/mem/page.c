#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <kernel/mem/page.h>
#include <kernel/mem/page_frame_allocator.h>

#define PML4_BASE ((uint64_t*) 0xFFFFFFFFFFFFF000)
#define PDP_BASE ((uint64_t*) 0xFFFFFFFFFFE00000)
#define PD_BASE ((uint64_t*) 0xFFFFFFFFC0000000)
#define PT_BASE ((uint64_t*) 0xFFFFFF8000000000)

static inline void invlpg(uint64_t addr) {
    asm volatile( "invlpg (%0)" : : "b"(addr) : "memory" );
}

static uintptr_t get_phys_addr(uintptr_t virt_addr) {
    uint64_t pml4_offset = (virt_addr >> 39) & 0x1FF;
    uint64_t pdp_offset = (virt_addr >> 30) & 0x1FF;
    uint64_t pd_offset = (virt_addr >> 21) & 0x1FF;
    uint64_t pt_offset = (virt_addr >> 12) & 0x1FF;

    uint64_t *pt_entry = PT_BASE + (0x40000000 * pml4_offset + 0x200000 * pdp_offset + 0x1000 * pd_offset) / sizeof(uint64_t) + pt_offset;
    return *pt_entry & 0x000FFFFFFFFFF000;
}

static bool all_empty(uint64_t *page) {
    for (size_t i = 0; i < PAGE_SIZE / sizeof(uint64_t); i++) {
        if (page[i] & 1) {
            return false;
        }
    }
    return true;
}

static void do_unmap_page(uintptr_t virt_addr, bool free_phys) {
    uint64_t pml4_offset = (virt_addr >> 39) & 0x1FF;
    uint64_t pdp_offset = (virt_addr >> 30) & 0x1FF;
    uint64_t pd_offset = (virt_addr >> 21) & 0x1FF;
    uint64_t pt_offset = (virt_addr >> 12) & 0x1FF;

    uint64_t *pml4 = PML4_BASE;
    uint64_t *pdp = PDP_BASE + (0x1000 * pml4_offset) / sizeof(uint64_t);
    uint64_t *pd = PD_BASE + (0x200000 * pml4_offset + 0x1000 * pdp_offset) / sizeof(uint64_t);
    uint64_t *pt = PT_BASE + (0x40000000 * pml4_offset + 0x200000L * pdp_offset + 0x1000 * pd_offset) / sizeof(uint64_t);

    if (free_phys) {
        free_phys_page(get_phys_addr(virt_addr));
    }
    pt[pt_offset] = 0;
    invlpg(virt_addr);

    if (all_empty(pt)) {
        if (free_phys) {
            free_phys_page(get_phys_addr((uintptr_t) pt));
        }
        pd[pd_offset] = 0;
    }

    if (all_empty(pd)) {
        if (free_phys) {
            free_phys_page(get_phys_addr((uintptr_t) pd));
        }
        pdp[pdp_offset] = 0;
    }

    if (all_empty(pdp)) {
        if (free_phys) {
            free_phys_page(get_phys_addr((uintptr_t) pdp));
        }
        pml4[pml4_offset] = 0;
    }
}

void clear_initial_page_mappings() {
    for (size_t i = 0; i < 0x600000; i += PAGE_SIZE) {
        do_unmap_page(i, false);
    }
}

void map_page(uint64_t virt_addr) {
    map_phys_page(get_next_phys_page(), virt_addr);
}

void map_phys_page(uintptr_t phys_addr, uintptr_t virt_addr) {
    uint64_t pml4_offset = (virt_addr >> 39) & 0x1FF;
    uint64_t pdp_offset = (virt_addr >> 30) & 0x1FF;
    uint64_t pd_offset = (virt_addr >> 21) & 0x1FF;
    uint64_t pt_offset = (virt_addr >> 12) & 0x1FF;

    uint64_t *pml4_entry = PML4_BASE + pml4_offset;
    uint64_t *pdp_entry = PDP_BASE + (0x1000 * pml4_offset) / sizeof(uint64_t) + pdp_offset;
    uint64_t *pd_entry = PD_BASE + (0x200000 * pml4_offset + 0x1000 * pdp_offset) / sizeof(uint64_t) + pd_offset;
    uint64_t *pt_entry = PT_BASE + (0x40000000 * pml4_offset + 0x200000 * pdp_offset + 0x1000 * pd_offset) / sizeof(uint64_t) + pt_offset;

    if (!(*pml4_entry & 1)) {
        *pml4_entry = get_next_phys_page() | 0x03;
        memset(pdp_entry - pdp_offset, 0, PAGE_SIZE);
    }

    if (!(*pdp_entry & 1)) {
        *pdp_entry = get_next_phys_page() | 0x03;
        memset(pd_entry - pd_offset, 0, PAGE_SIZE);
    }

    if (!(*pd_entry & 1)) {
        *pd_entry = get_next_phys_page() | 0x03;
        memset(pt_entry - pt_offset, 0, PAGE_SIZE);
    }

    if (*pt_entry & 1) {
        invlpg(virt_addr);
    }
    *pt_entry = phys_addr | 0x03;
}

void unmap_page(uintptr_t virt_addr) {
    do_unmap_page(virt_addr, true);
}