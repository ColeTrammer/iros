#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include <kernel/mem/page.h>
#include <kernel/mem/page_frame_allocator.h>
#include <kernel/mem/vm_region.h>
#include <kernel/hal/output.h>
#include <kernel/proc/process.h>

#include <kernel/arch/x86_64/mem/page.h>
#include <kernel/arch/x86_64/asm_utils.h>
#include <kernel/hal/x86_64/drivers/vga.h>

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
    uint64_t *pt = PT_BASE + (0x40000000 * pml4_offset + 0x200000 * pdp_offset + 0x1000 * pd_offset) / sizeof(uint64_t);

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
    update_vga_buffer();

    for (size_t i = 0; i < 0x600000; i += PAGE_SIZE) {
        do_unmap_page(i, false);
    }
}

void map_page(uintptr_t virt_addr, uint64_t flags) {
    map_phys_page(get_next_phys_page(), virt_addr, flags);
}

void map_phys_page(uintptr_t phys_addr, uintptr_t virt_addr, uint64_t flags) {
    flags &= (VM_WRITE | VM_USER | VM_GLOBAL | VM_NO_EXEC);
    flags |= 0x01;

    uint64_t pml4_offset = (virt_addr >> 39) & 0x1FF;
    uint64_t pdp_offset = (virt_addr >> 30) & 0x1FF;
    uint64_t pd_offset = (virt_addr >> 21) & 0x1FF;
    uint64_t pt_offset = (virt_addr >> 12) & 0x1FF;

    uint64_t *pml4_entry = PML4_BASE + pml4_offset;
    uint64_t *pdp_entry = PDP_BASE + (0x1000 * pml4_offset) / sizeof(uint64_t) + pdp_offset;
    uint64_t *pd_entry = PD_BASE + (0x200000 * pml4_offset + 0x1000 * pdp_offset) / sizeof(uint64_t) + pd_offset;
    uint64_t *pt_entry = PT_BASE + (0x40000000 * pml4_offset + 0x200000 * pdp_offset + 0x1000 * pd_offset) / sizeof(uint64_t) + pt_offset;

    if (!(*pml4_entry & 1)) {
        *pml4_entry = get_next_phys_page() | VM_WRITE | (VM_USER & flags) | 0x01;
        memset(pdp_entry - pdp_offset, 0, PAGE_SIZE);
    }

    if (!(*pdp_entry & 1)) {
        *pdp_entry = get_next_phys_page() | VM_WRITE | (VM_USER & flags) | 0x01;
        memset(pd_entry - pd_offset, 0, PAGE_SIZE);
    }

    if (!(*pd_entry & 1)) {
        *pd_entry = get_next_phys_page() | VM_WRITE | (VM_USER & flags) | 0x01;
        memset(pt_entry - pt_offset, 0, PAGE_SIZE);
    }

    if (*pt_entry & 1) {
        invlpg(virt_addr);
    }
    *pt_entry = phys_addr | flags;
}

void unmap_page(uintptr_t virt_addr) {
    do_unmap_page(virt_addr, true);
}

uintptr_t create_paging_structure(struct vm_region *list, bool deep_copy) {
    map_page((uintptr_t) TEMP_PAGE, VM_WRITE);
    uint64_t *pml4 = TEMP_PAGE;

    for (uint64_t i = 0; i < MAX_PML4_ENTRIES - 1; i++) {
        pml4[i] = PML4_BASE[i];
    }

    pml4[MAX_PML4_ENTRIES - 1] = get_phys_addr((uintptr_t) pml4) | PAGE_STRUCTURE_FLAGS | VM_NO_EXEC;

    load_cr3(get_phys_addr((uintptr_t) pml4));

    if (deep_copy) {
        for (uint64_t i = 0; i < MAX_PML4_ENTRIES - 1; i++) {
            if (PML4_BASE[i] != 0) {
                for (uint64_t j = 0; j < MAX_PDP_ENTRIES; j++) {
                    uint64_t *pdp = PDP_BASE + (0x1000 * i) / sizeof(uint64_t);
                    if (pdp[j] != 0) {
                        for (uint64_t k = 0; k < MAX_PD_ENTRIES; k++) {
                            uint64_t *pd = PD_BASE + (0x200000 * i + 0x1000 * j) / sizeof(uint64_t);
                            if (pd[k] != 0) {
                                map_page((uintptr_t) TEMP_PAGE, VM_WRITE);
                                uint64_t *pt = TEMP_PAGE;
                                uint64_t *old_pt = PT_BASE + (0x40000000 * i + 0x200000 * j + 0x1000 * k) / sizeof(uint64_t);
                                for (uint64_t l = 0; l < MAX_PT_ENTRIES; l++) {
                                    pt[l] = old_pt[l];
                                }
                                pd[k] = get_phys_addr((uintptr_t) pt) | PAGE_STRUCTURE_FLAGS;
                                invlpg((uintptr_t) old_pt);
                            }
                        }

                        map_page((uintptr_t) TEMP_PAGE, VM_WRITE);
                        uint64_t *pd = TEMP_PAGE;
                        uint64_t *old_pd = PD_BASE + (0x200000 * i + 0x1000 * j) / sizeof(uint64_t);
                        for (uint64_t k = 0; k < MAX_PD_ENTRIES; k++) {
                            pd[k] = old_pd[k];
                        }
                        pdp[j] = get_phys_addr((uintptr_t) pd) | PAGE_STRUCTURE_FLAGS;
                        invlpg((uintptr_t) old_pd);
                    }
                }

                map_page((uintptr_t) TEMP_PAGE, VM_WRITE);
                uint64_t *pdp = TEMP_PAGE;
                uint64_t *old_pdp = PDP_BASE + (0x1000 * i) / sizeof(uint64_t);
                for (uint64_t j = 0; j < MAX_PDP_ENTRIES; j++) {
                    pdp[j] = old_pdp[j];
                }
                PML4_BASE[i] = get_phys_addr((uintptr_t) pdp) | PAGE_STRUCTURE_FLAGS;
                invlpg((uintptr_t) old_pdp);
            }
        }

        while (list != NULL) {
            map_vm_region_flags(list);
            list = list->next;
        }
    }

    return get_phys_addr((uintptr_t) PML4_BASE);
}

void load_paging_structure(uintptr_t phys_addr) {
    load_cr3(phys_addr);
}

void remove_paging_structure(uintptr_t phys_addr, struct vm_region *list) {
    load_cr3(phys_addr);

    struct vm_region *region = list;
    while (region != NULL) {
        for (uintptr_t page = region->start; page < region->end; page += PAGE_SIZE) {
            unmap_page(page);
        }
        region = region->next;
    }

    load_cr3(get_current_process()->arch_process.cr3);
    free_phys_page(phys_addr);
}

void map_vm_region_flags(struct vm_region *region) {
    for (uintptr_t addr = region->start; addr < region->end; addr += PAGE_SIZE) {
        map_phys_page(get_phys_addr(addr), addr, region->flags);
    }
}

void map_vm_region(struct vm_region *region) {
    for (uintptr_t addr = region->start; addr < region->end; addr += PAGE_SIZE) {
        map_page(addr, region->flags);
        debug_log("Mapped VM Region: [ %#.16lX, %#.16lX, %#.16lX, %#.16lX ]\n", region->type, region->flags, region->start, region->end);
    }    
}