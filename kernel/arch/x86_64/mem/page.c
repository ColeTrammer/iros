#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/hal/output.h>
#include <kernel/mem/kernel_vm.h>
#include <kernel/mem/page.h>
#include <kernel/mem/page_frame_allocator.h>
#include <kernel/mem/vm_region.h>
#include <kernel/proc/task.h>

#include <kernel/arch/x86_64/asm_utils.h>
#include <kernel/arch/x86_64/mem/page.h>
#include <kernel/hal/x86_64/drivers/vga.h>

// #define MAP_VM_REGION_DEBUG

static spinlock_t temp_page_lock = SPINLOCK_INITIALIZER;

uintptr_t get_phys_addr(uintptr_t virt_addr) {
    uint64_t pml4_offset = (virt_addr >> 39) & 0x1FF;
    uint64_t pdp_offset = (virt_addr >> 30) & 0x1FF;
    uint64_t pd_offset = (virt_addr >> 21) & 0x1FF;
    uint64_t pt_offset = (virt_addr >> 12) & 0x1FF;

    uint64_t *pml4 = PML4_BASE;
    uint64_t *pdp = PDP_BASE + (0x1000 * pml4_offset) / sizeof(uint64_t);
    uint64_t *pd = PD_BASE + (0x200000 * pml4_offset + 0x1000 * pdp_offset) / sizeof(uint64_t);
    uint64_t *pt = PT_BASE + (0x40000000 * pml4_offset + 0x200000 * pdp_offset + 0x1000 * pd_offset) / sizeof(uint64_t);

    assert(pml4[pml4_offset] & 1);
    assert(pdp[pdp_offset] & 1);
    assert(pd[pd_offset] & 1);
    assert(pt[pt_offset] & 1);

    uint64_t *pt_entry = pt + pt_offset;
    return (*pt_entry & 0x0000FFFFFFFFF000ULL) + (virt_addr & 0xFFF);
}

static bool all_empty(uint64_t *page) {
    for (size_t i = 0; i < PAGE_SIZE / sizeof(uint64_t); i++) {
        if (page[i] & 1) {
            return false;
        }
    }
    return true;
}

void do_unmap_page(uintptr_t virt_addr, bool free_phys) {
    uint64_t pml4_offset = (virt_addr >> 39) & 0x1FF;
    uint64_t pdp_offset = (virt_addr >> 30) & 0x1FF;
    uint64_t pd_offset = (virt_addr >> 21) & 0x1FF;
    uint64_t pt_offset = (virt_addr >> 12) & 0x1FF;

    uint64_t *pml4 = PML4_BASE;
    uint64_t *pdp = PDP_BASE + (0x1000 * pml4_offset) / sizeof(uint64_t);
    uint64_t *pd = PD_BASE + (0x200000 * pml4_offset + 0x1000 * pdp_offset) / sizeof(uint64_t);
    uint64_t *pt = PT_BASE + (0x40000000 * pml4_offset + 0x200000 * pdp_offset + 0x1000 * pd_offset) / sizeof(uint64_t);

    if (!(pml4[pml4_offset] & 1) || !(pdp[pdp_offset] & 1) || !(pd[pd_offset] & 1) || !(pt[pt_offset] & 1)) {
        return; // Page is already unmapped
    }

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

static void do_map_phys_page(uintptr_t phys_addr, uintptr_t virt_addr, uint64_t flags, struct virt_page_info *info) {
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
        invlpg((uintptr_t) pdp_entry);
        memset(pdp_entry - pdp_offset, 0, PAGE_SIZE);
    }

    if (!(*pdp_entry & 1)) {
        *pdp_entry = get_next_phys_page() | VM_WRITE | (VM_USER & flags) | 0x01;
        invlpg((uintptr_t) pd_entry);
        memset(pd_entry - pd_offset, 0, PAGE_SIZE);
    }

    if (!(*pd_entry & 1)) {
        *pd_entry = get_next_phys_page() | VM_WRITE | (VM_USER & flags) | 0x01;
        invlpg((uintptr_t) pt_entry);
        memset(pt_entry - pt_offset, 0, PAGE_SIZE);
    }

    if (*pt_entry & 1) {
        invlpg(virt_addr);
    }
    *pt_entry = phys_addr | flags;

    if (info != NULL) {
        info->pml4_index = pml4_offset;
        info->pdp_index = pdp_offset;
        info->pd_index = pd_offset;
        info->pt_index = pt_offset;
        info->pml4_entry = *pml4_entry;
        info->pdp_entry = *pdp_entry;
        info->pd_entry = *pd_entry;
        info->pt_entry = *pt_entry;
    }
}

void map_page_info(struct virt_page_info *info) {
    uint64_t *pml4_entry = PML4_BASE + info->pml4_index;
    uint64_t *pdp_entry = PDP_BASE + (0x1000 * info->pml4_index) / sizeof(uint64_t) + info->pdp_index;
    uint64_t *pd_entry = PD_BASE + (0x200000 * info->pml4_index + 0x1000 * info->pdp_index) / sizeof(uint64_t) + info->pd_index;
    uint64_t *pt_entry = PT_BASE + (0x40000000 * info->pml4_index + 0x200000 * info->pdp_index + 0x1000 * info->pd_index) / sizeof(uint64_t)
        + info->pt_index;

    if ((*pml4_entry & ~0xFFF) != (info->pml4_entry & ~0xFFF)) {
        *pml4_entry = info->pml4_entry;
    }

    if ((*pdp_entry & ~0xFFF) != (info->pdp_entry & ~0xFFF)) {
        *pdp_entry = info->pdp_entry;
    }

    if ((*pd_entry & ~0xFFF) != (info->pd_entry & ~0xFFF)) {
        *pd_entry = info->pd_entry;
    }

    if ((*pt_entry & ~0xFFF) != (info->pt_entry & ~0xFFF)) {
        *pt_entry = info->pt_entry;
    }
}

void clear_initial_page_mappings() {
    update_vga_buffer();

    for (size_t i = 0; i < 0x600000; i += PAGE_SIZE) {
        do_unmap_page(i, false);
    }
}

void map_page(uintptr_t virt_addr, uint64_t flags) {
    do_map_phys_page(get_next_phys_page(), virt_addr, flags, NULL);
}

void map_phys_page(uintptr_t phys_addr, uintptr_t virt_addr, uint64_t flags) {
    do_map_phys_page(phys_addr, virt_addr, flags, NULL);
}

struct virt_page_info *map_page_with_info(uintptr_t virt_addr, uint64_t flags) {
    struct virt_page_info *info = calloc(1, sizeof(struct virt_page_info));
    do_map_phys_page(get_next_phys_page(), virt_addr, flags, info);
    return info;
}

void unmap_page(uintptr_t virt_addr) {
    do_unmap_page(virt_addr, true);
}

uintptr_t get_current_paging_structure() {
    return get_current_task()->process->arch_process.cr3;
}

uintptr_t clone_process_paging_structure() {
    spin_lock(&temp_page_lock);

    map_page((uintptr_t) TEMP_PAGE, VM_WRITE);
    uint64_t *pml4 = TEMP_PAGE;

    for (uint64_t i = 0; i < MAX_PML4_ENTRIES - 1; i++) {
        pml4[i] = PML4_BASE[i];
    }

    pml4[MAX_PML4_ENTRIES - 1] = get_phys_addr((uintptr_t) pml4) | PAGE_STRUCTURE_FLAGS | VM_NO_EXEC;
    uintptr_t pml4_addr = get_phys_addr((uintptr_t) TEMP_PAGE);

    uint64_t old_cr3 = get_cr3();
    get_current_task()->process->arch_process.cr3 = get_phys_addr((uintptr_t) pml4);
    load_cr3(get_phys_addr((uintptr_t) pml4));

    for (uint64_t i = 0; i < MAX_PML4_ENTRIES - 3; i++) {
        if (PML4_BASE[i] != 0) {
            map_page((uintptr_t) TEMP_PAGE, VM_WRITE);
            uint64_t *temp_pdp = TEMP_PAGE;
            uint64_t *pdp = PDP_BASE + (0x1000 * i) / sizeof(uint64_t);
            for (uint64_t j = 0; j < MAX_PDP_ENTRIES; j++) {
                temp_pdp[j] = pdp[j];
            }
            PML4_BASE[i] = get_phys_addr((uintptr_t) temp_pdp) | PAGE_STRUCTURE_FLAGS | VM_USER;
            invlpg((uintptr_t) pdp);

            for (uint64_t j = 0; j < MAX_PDP_ENTRIES; j++) {
                if (pdp[j] != 0) {
                    map_page((uintptr_t) TEMP_PAGE, VM_WRITE);
                    uint64_t *temp_pd = TEMP_PAGE;
                    uint64_t *pd = PD_BASE + (0x200000 * i + 0x1000 * j) / sizeof(uint64_t);
                    for (uint64_t k = 0; k < MAX_PD_ENTRIES; k++) {
                        temp_pd[k] = pd[k];
                    }
                    pdp[j] = get_phys_addr((uintptr_t) temp_pd) | PAGE_STRUCTURE_FLAGS | VM_USER;
                    invlpg((uintptr_t) pd);

                    for (uint64_t k = 0; k < MAX_PD_ENTRIES; k++) {
                        uint64_t *pd = PD_BASE + (0x200000 * i + 0x1000 * j) / sizeof(uint64_t);
                        if (pd[k] != 0) {
                            map_page((uintptr_t) TEMP_PAGE, VM_WRITE);
                            uint64_t *temp_pt = TEMP_PAGE;
                            uint64_t *pt = PT_BASE + (0x40000000 * i + 0x200000 * j + 0x1000 * k) / sizeof(uint64_t);
                            for (uint64_t l = 0; l < MAX_PT_ENTRIES; l++) {
                                temp_pt[l] = pt[l];
                            }
                            pd[k] = get_phys_addr((uintptr_t) temp_pt) | PAGE_STRUCTURE_FLAGS | VM_USER;
                            invlpg((uintptr_t) pt);

                            for (uint64_t l = 0; l < MAX_PT_ENTRIES; l++) {
                                if (pt[l] != 0) {
                                    map_page((uintptr_t) TEMP_PAGE, VM_WRITE);
                                    uint64_t *temp_page = TEMP_PAGE;
                                    uint64_t *page = (uint64_t *) VIRT_ADDR(i, j, k, l);
                                    memcpy(temp_page, page, PAGE_SIZE);
                                    pt[l] = get_phys_addr((uintptr_t) temp_page) | (pt[l] & 0xFFFF000000000FFFUL);
                                    invlpg((uintptr_t) page);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    spin_unlock(&temp_page_lock);

    get_current_task()->process->arch_process.cr3 = old_cr3;
    load_cr3(old_cr3);

    return pml4_addr;
}

uintptr_t create_paging_structure(struct vm_region *list, bool deep_copy) {
    spin_lock(&temp_page_lock);

    map_page((uintptr_t) TEMP_PAGE, VM_WRITE);
    uint64_t *pml4 = TEMP_PAGE;

    for (uint64_t i = 0; i < MAX_PML4_ENTRIES - 1; i++) {
        pml4[i] = PML4_BASE[i];
    }

    pml4[MAX_PML4_ENTRIES - 1] = get_phys_addr((uintptr_t) pml4) | PAGE_STRUCTURE_FLAGS | VM_NO_EXEC;
    uintptr_t pml4_addr = get_phys_addr((uintptr_t) TEMP_PAGE);

    if (deep_copy) {
        uint64_t old_cr3 = get_cr3();
        get_current_task()->process->arch_process.cr3 = get_phys_addr((uintptr_t) pml4);
        load_cr3(get_phys_addr((uintptr_t) pml4));

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
            if (list->type != VM_KERNEL_PHYS_ID) {
                map_vm_region_flags(list);
            }
            list = list->next;
        }

        get_current_task()->process->arch_process.cr3 = old_cr3;
        load_cr3(old_cr3);
    }

    spin_unlock(&temp_page_lock);

    return pml4_addr;
}

void create_phys_id_map() {
    // Map entries at PML4_MAX - 3 to a replica of phys memory
    debug_log("Mapping physical address identity map: [ %#lX ]\n", get_total_phys_memory());
    for (uintptr_t i = 0; i < get_total_phys_memory(); i += PAGE_SIZE) {
        map_phys_page(i, VIRT_ADDR(MAX_PML4_ENTRIES - 3UL, 0, 0, 0) + i, VM_WRITE | VM_GLOBAL | VM_NO_EXEC);
    }
}

void load_paging_structure(uintptr_t phys_addr) {
    get_current_task()->process->arch_process.cr3 = phys_addr;
    load_cr3(phys_addr);
}

void soft_remove_paging_structure(struct vm_region *list) {
    struct vm_region *region = list;
    while (region != NULL) {
        if (!(region->flags & VM_GLOBAL) && !(region->type == VM_KERNEL_STACK) && !(region->type == VM_TASK_STACK)) {
            for (uintptr_t page = region->start; page < region->end; page += PAGE_SIZE) {
                unmap_page(page);
            }
        }
        region = region->next;
    }
}

/* Must be called from unpremptable context */
void remove_paging_structure(uintptr_t phys_addr, struct vm_region *list) {
    uint64_t old_cr3 = get_cr3();
    load_cr3(phys_addr);

    struct vm_region *region = list;
    while (region != NULL) {
        if (!(region->flags & VM_GLOBAL)) {
            for (uintptr_t page = region->start; page < region->end; page += PAGE_SIZE) {
                if (region->type == VM_KERNEL_STACK) {
                    // NOTE: This section should be removed separately, when an individual task
                    //       ends. This is because each and every task gets its own kernel stack,
                    //       but they share the rest of the memory. Since this method should only
                    //       be called when all tasks are gone, unmapping the pages in the region
                    //       would either do nothing or cause a page fault, and thus should be
                    //       avoided.
                    continue;
                } else if (region->type == VM_DEVICE_MEMORY_MAP_DONT_FREE_PHYS_PAGES) {
                    do_unmap_page(page, false);
                } else {
                    unmap_page(page);
                }
            }
        }
        region = region->next;
    }

    load_cr3(old_cr3);
    free_phys_page(phys_addr);
}

void map_vm_region_flags(struct vm_region *region) {
    for (uintptr_t addr = region->start; addr < region->end; addr += PAGE_SIZE) {
        map_phys_page(get_phys_addr(addr), addr, region->flags);
    }
}

void map_vm_region(struct vm_region *region) {
#ifdef MAP_VM_REGION_DEBUG
    debug_log("Mapped VM Region: [ %#.16lX, %#.16lX, %#.16lX, %#.16lX, %#.16lX ]\n", get_cr3(), region->type, region->flags, region->start,
        region->end);
#endif /* MAP_VM_REGION_DEBUG */
    for (uintptr_t addr = region->start; addr < region->end; addr += PAGE_SIZE) {
        map_page(addr, region->flags);
    }
}