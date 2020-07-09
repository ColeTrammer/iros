#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/hal/output.h>
#include <kernel/hal/processor.h>
#include <kernel/mem/kernel_vm.h>
#include <kernel/mem/page.h>
#include <kernel/mem/page_frame_allocator.h>
#include <kernel/mem/vm_region.h>
#include <kernel/proc/task.h>

#include <kernel/arch/x86_64/asm_utils.h>
#include <kernel/hal/x86_64/drivers/vga.h>

// #define MAP_VM_REGION_DEBUG

spinlock_t temp_page_lock = SPINLOCK_INITIALIZER;
extern struct process initial_kernel_process;

void flush_tlb(uintptr_t addr) {
    invlpg(addr);
    broadcast_flush_tlb(addr, 1);
}

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

void do_unmap_page(uintptr_t virt_addr, bool free_phys, bool free_phys_structure, bool broadcast_tlb_flush, struct process *process) {
    void (*do_tlb_flush)(uintptr_t) = broadcast_tlb_flush ? &flush_tlb : &invlpg;

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
        free_phys_page(get_phys_addr(virt_addr), process);
    }
    pt[pt_offset] = 0;
    do_tlb_flush(virt_addr);

    if (all_empty(pt)) {
        if (free_phys_structure) {
            free_phys_page(get_phys_addr((uintptr_t) pt), process);
        }
        pd[pd_offset] = 0;
        do_tlb_flush((uintptr_t) &pt[pt_offset]);
    }

    if (all_empty(pd)) {
        if (free_phys_structure) {
            free_phys_page(get_phys_addr((uintptr_t) pd), process);
        }
        pdp[pdp_offset] = 0;
        do_tlb_flush((uintptr_t) &pd[pd_offset]);
    }

    if (all_empty(pdp)) {
        if (free_phys_structure) {
            free_phys_page(get_phys_addr((uintptr_t) pdp), process);
        }
        pml4[pml4_offset] = 0;
        do_tlb_flush((uintptr_t) &pdp[pdp_offset]);
    }
}

void do_map_phys_page(uintptr_t phys_addr, uintptr_t virt_addr, uint64_t flags, bool broadcast_flush_tlb, struct process *process) {
    void (*do_tlb_flush)(uintptr_t) = broadcast_flush_tlb ? &flush_tlb : &invlpg;

    flags &= (VM_WRITE | VM_USER | VM_GLOBAL | VM_NO_EXEC | VM_COW | VM_SHARED | VM_PROT_NONE);
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
        *pml4_entry = get_next_phys_page(process) | VM_WRITE | (VM_USER & flags) | 0x01;
        do_tlb_flush((uintptr_t) pdp_entry);
        memset(pdp_entry - pdp_offset, 0, PAGE_SIZE);
    }

    if (!(*pdp_entry & 1)) {
        *pdp_entry = get_next_phys_page(process) | VM_WRITE | (VM_USER & flags) | 0x01;
        do_tlb_flush((uintptr_t) pd_entry);
        memset(pd_entry - pd_offset, 0, PAGE_SIZE);
    }

    if (!(*pd_entry & 1)) {
        *pd_entry = get_next_phys_page(process) | VM_WRITE | (VM_USER & flags) | 0x01;
        do_tlb_flush((uintptr_t) pt_entry);
        memset(pt_entry - pt_offset, 0, PAGE_SIZE);
    }

    *pt_entry = phys_addr | flags;
    if (*pt_entry & 1) {
        do_tlb_flush(virt_addr);
    }
}

void map_page_flags(uintptr_t virt_addr, uint64_t flags) {
    uint64_t pml4_offset = (virt_addr >> 39) & 0x1FF;
    uint64_t pdp_offset = (virt_addr >> 30) & 0x1FF;
    uint64_t pd_offset = (virt_addr >> 21) & 0x1FF;
    uint64_t pt_offset = (virt_addr >> 12) & 0x1FF;

    uint64_t *pml4_entry = PML4_BASE + pml4_offset;
    uint64_t *pdp_entry = PDP_BASE + (0x1000 * pml4_offset) / sizeof(uint64_t) + pdp_offset;
    uint64_t *pd_entry = PD_BASE + (0x200000 * pml4_offset + 0x1000 * pdp_offset) / sizeof(uint64_t) + pd_offset;
    uint64_t *pt_entry = PT_BASE + (0x40000000 * pml4_offset + 0x200000 * pdp_offset + 0x1000 * pd_offset) / sizeof(uint64_t) + pt_offset;

    if (!(*pml4_entry & 1) || !(*pdp_entry & 1) || !(*pd_entry & 1) || !(*pt_entry & 1)) {
        return; // Page is isn't mapped, so will be mapped later on page fault and we can ignore it for now
    }

    flags |= (flags & VM_PROT_NONE ? 0x01 : 0);
    flags &= (VM_WRITE | VM_USER | VM_GLOBAL | VM_NO_EXEC | VM_COW | VM_SHARED | VM_PROT_NONE);
    *pt_entry |= flags;
    flush_tlb(virt_addr);
}

uint64_t *get_page_table_entry(uintptr_t virt_addr) {
    virt_addr &= ~0xFFF;

    uint64_t pml4_offset = (virt_addr >> 39) & 0x1FF;
    uint64_t pdp_offset = (virt_addr >> 30) & 0x1FF;
    uint64_t pd_offset = (virt_addr >> 21) & 0x1FF;
    uint64_t pt_offset = (virt_addr >> 12) & 0x1FF;

    uint64_t *pml4_entry = PML4_BASE + pml4_offset;
    uint64_t *pdp_entry = PDP_BASE + (0x1000 * pml4_offset) / sizeof(uint64_t) + pdp_offset;
    uint64_t *pd_entry = PD_BASE + (0x200000 * pml4_offset + 0x1000 * pdp_offset) / sizeof(uint64_t) + pd_offset;
    uint64_t *pt_entry = PT_BASE + (0x40000000 * pml4_offset + 0x200000 * pdp_offset + 0x1000 * pd_offset) / sizeof(uint64_t) + pt_offset;

    if (!(*pml4_entry & 1) || !(*pdp_entry & 1) || !(*pd_entry & 1)) {
        return NULL;
    }

    return pt_entry;
}

bool is_virt_addr_cow(uintptr_t virt_addr) {
    uint64_t *pt_entry = get_page_table_entry(virt_addr);
    return pt_entry && (*pt_entry & VM_COW);
}

void clear_initial_page_mappings() {
    update_vga_buffer();

    for (size_t i = 0; i < 0x600000; i += PAGE_SIZE) {
        do_unmap_page(i, false, false, false, &initial_kernel_process);
    }
}

void map_page(uintptr_t virt_addr, uint64_t flags, struct process *process) {
    do_map_phys_page(get_next_phys_page(process), virt_addr, flags, true, process);
}

void map_phys_page(uintptr_t phys_addr, uintptr_t virt_addr, uint64_t flags, struct process *process) {
    do_map_phys_page(phys_addr, virt_addr, flags, true, process);
}

void unmap_page(uintptr_t virt_addr, struct process *process) {
    do_unmap_page(virt_addr, true, true, true, process);
}

uintptr_t get_current_paging_structure() {
    return get_current_task()->process->arch_process.cr3;
}

uintptr_t create_paging_structure(struct vm_region *list, bool deep_copy, struct process *process) {
    spin_lock(&temp_page_lock);

    map_page((uintptr_t) TEMP_PAGE, VM_WRITE, process);
    uint64_t *pml4 = TEMP_PAGE;

    for (uint64_t i = 0; i < MAX_PML4_ENTRIES - 1; i++) {
        pml4[i] = PML4_BASE[i];
    }

    pml4[MAX_PML4_ENTRIES - 1] = get_phys_addr((uintptr_t) pml4) | PAGE_STRUCTURE_FLAGS | VM_NO_EXEC;
    uintptr_t pml4_addr = get_phys_addr((uintptr_t) TEMP_PAGE);

    if (deep_copy) {
        uint64_t old_cr3 = get_cr3();
        process->arch_process.cr3 = get_phys_addr((uintptr_t) pml4);
        load_cr3(get_phys_addr((uintptr_t) pml4));

        for (uint64_t i = 0; i < MAX_PML4_ENTRIES - 1; i++) {
            if (PML4_BASE[i] != 0) {
                for (uint64_t j = 0; j < MAX_PDP_ENTRIES; j++) {
                    uint64_t *pdp = PDP_BASE + (0x1000 * i) / sizeof(uint64_t);
                    if (pdp[j] != 0) {
                        for (uint64_t k = 0; k < MAX_PD_ENTRIES; k++) {
                            uint64_t *pd = PD_BASE + (0x200000 * i + 0x1000 * j) / sizeof(uint64_t);
                            if (pd[k] != 0) {
                                map_page((uintptr_t) TEMP_PAGE, VM_WRITE, process);
                                uint64_t *pt = TEMP_PAGE;
                                uint64_t *old_pt = PT_BASE + (0x40000000 * i + 0x200000 * j + 0x1000 * k) / sizeof(uint64_t);
                                for (uint64_t l = 0; l < MAX_PT_ENTRIES; l++) {
                                    pt[l] = old_pt[l];
                                }
                                pd[k] = get_phys_addr((uintptr_t) pt) | PAGE_STRUCTURE_FLAGS;
                                flush_tlb((uintptr_t) old_pt);
                            }
                        }

                        map_page((uintptr_t) TEMP_PAGE, VM_WRITE, process);
                        uint64_t *pd = TEMP_PAGE;
                        uint64_t *old_pd = PD_BASE + (0x200000 * i + 0x1000 * j) / sizeof(uint64_t);
                        for (uint64_t k = 0; k < MAX_PD_ENTRIES; k++) {
                            pd[k] = old_pd[k];
                        }
                        pdp[j] = get_phys_addr((uintptr_t) pd) | PAGE_STRUCTURE_FLAGS;
                        flush_tlb((uintptr_t) old_pd);
                    }
                }

                map_page((uintptr_t) TEMP_PAGE, VM_WRITE, process);
                uint64_t *pdp = TEMP_PAGE;
                uint64_t *old_pdp = PDP_BASE + (0x1000 * i) / sizeof(uint64_t);
                for (uint64_t j = 0; j < MAX_PDP_ENTRIES; j++) {
                    pdp[j] = old_pdp[j];
                }
                PML4_BASE[i] = get_phys_addr((uintptr_t) pdp) | PAGE_STRUCTURE_FLAGS;
                flush_tlb((uintptr_t) old_pdp);
            }
        }

        while (list != NULL) {
            if (list->type != VM_KERNEL_PHYS_ID) {
                map_vm_region_flags(list, process);
            }
            list = list->next;
        }

        process->arch_process.cr3 = old_cr3;
        load_cr3(old_cr3);
    }

    spin_unlock(&temp_page_lock);

    return pml4_addr;
}

uintptr_t create_clone_process_paging_structure(struct process *process) {
    spin_lock(&temp_page_lock);

    map_page((uintptr_t) TEMP_PAGE, VM_WRITE, process);
    uint64_t *pml4 = TEMP_PAGE;

    // Only clone the kernel entries in this table.
    memset(pml4, 0, (MAX_PML4_ENTRIES - 3) * sizeof(uint64_t));
    pml4[MAX_PML4_ENTRIES - 3] = PML4_BASE[MAX_PML4_ENTRIES - 3];
    pml4[MAX_PML4_ENTRIES - 2] = PML4_BASE[MAX_PML4_ENTRIES - 2];
    pml4[MAX_PML4_ENTRIES - 1] = get_phys_addr((uintptr_t) pml4) | PAGE_STRUCTURE_FLAGS | VM_NO_EXEC;
    uintptr_t pml4_addr = get_phys_addr((uintptr_t) TEMP_PAGE);

    uint64_t old_cr3 = get_cr3();
    uint64_t flags = disable_interrupts_save();

    load_cr3(pml4_addr);

    struct vm_region *region = process->process_memory;
    while (region) {
        if (region->vm_object) {
            vm_map_region_with_object(region);
        }
        region = region->next;
    }

    load_cr3(old_cr3);
    interrupts_restore(flags);
    spin_unlock(&temp_page_lock);
    return pml4_addr;
}

void create_phys_id_map() {
    // Map entries at PML4_MAX - 3 to a replica of phys memory
    debug_log("Mapping physical address identity map: [ %#lX ]\n", get_max_phys_memory());
    for (uintptr_t i = 0; i < get_max_phys_memory(); i += PAGE_SIZE) {
        map_phys_page(i, VIRT_ADDR(MAX_PML4_ENTRIES - 3UL, 0, 0, 0) + i, VM_WRITE | VM_GLOBAL | VM_NO_EXEC, &initial_kernel_process);
    }
}

void load_paging_structure(uintptr_t phys_addr, struct process *process) {
    process->arch_process.cr3 = phys_addr;
    load_cr3(phys_addr);
}

void soft_remove_paging_structure(struct vm_region *list) {
    struct vm_region *region = list;
    while (region != NULL) {
        if (!(region->flags & VM_GLOBAL)) {
            for (uintptr_t page = region->start; page < region->end; page += PAGE_SIZE) {
                // NOTE: The vm object is responsible for unmapping the physical pages
                do_unmap_page(page, false, true, false, NULL);
            }
        }
        region = region->next;
    }
}

void mark_region_as_cow(struct vm_region *region) {
    for (uintptr_t addr = region->start; addr < region->end; addr += PAGE_SIZE) {
        uint64_t *pt_entry = get_page_table_entry(addr);
        if (!pt_entry || !(*pt_entry & 1)) {
            continue;
        }

        *pt_entry |= VM_COW;
        *pt_entry &= ~VM_WRITE;
    }
}

void remove_paging_structure(uintptr_t phys_addr, struct vm_region *list) {
    // Disable interrupts since we have to change the value of CR3. This could potentially be avoided by
    // freeing the memory in old CR3 by traversing the physical addresses directly instead of using a
    // recursive page mapping.
    uint64_t interrupts_save = disable_interrupts_save();

    uint64_t old_cr3 = get_cr3();
    if (old_cr3 == phys_addr) {
        old_cr3 = initial_kernel_process.arch_process.cr3;
    } else {
        load_cr3(phys_addr);
    }

    soft_remove_paging_structure(list);

    load_cr3(old_cr3);
    free_phys_page(phys_addr, NULL);

    interrupts_restore(interrupts_save);
}

void map_vm_region_flags(struct vm_region *region, struct process *process) {
    for (uintptr_t addr = region->start; addr < region->end; addr += PAGE_SIZE) {
        map_phys_page(get_phys_addr(addr), addr, region->flags, process);
    }
}

void map_vm_region(struct vm_region *region, struct process *process) {
#ifdef MAP_VM_REGION_DEBUG
    debug_log("Mapped VM Region: [ %#.16lX, %#.16lX, %#.16lX, %#.16lX, %#.16lX ]\n", get_cr3(), region->type, region->flags, region->start,
              region->end);
#endif /* MAP_VM_REGION_DEBUG */
    for (uintptr_t addr = region->start; addr < region->end; addr += PAGE_SIZE) {
        map_page(addr, region->flags, process);
    }
}
