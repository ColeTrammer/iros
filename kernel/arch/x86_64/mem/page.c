#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/hal/hal.h>
#include <kernel/hal/output.h>
#include <kernel/hal/processor.h>
#include <kernel/mem/kernel_vm.h>
#include <kernel/mem/page.h>
#include <kernel/mem/page_frame_allocator.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/mem/vm_region.h>
#include <kernel/proc/process.h>
#include <kernel/proc/stats.h>
#include <kernel/proc/task.h>

#include <kernel/arch/x86_64/asm_utils.h>
#include <kernel/hal/x86_64/drivers/vga.h>

// #define MAP_VM_REGION_DEBUG

spinlock_t temp_page_lock = SPINLOCK_INITIALIZER;

void flush_tlb(uintptr_t addr) {
    invlpg(addr);
    broadcast_flush_tlb(addr, 1);
}

uintptr_t get_phys_addr(uintptr_t virt_addr) {
    uint64_t pml4_offset = (virt_addr >> 39) & 0x1FF;
    uint64_t pdp_offset = (virt_addr >> 30) & 0x1FF;
    uint64_t pd_offset = (virt_addr >> 21) & 0x1FF;
    uint64_t pt_offset = (virt_addr >> 12) & 0x1FF;

    uint64_t *pml4 = create_phys_addr_mapping(get_cr3() & 0x0000FFFFFFFFF000ULL);
    assert(pml4[pml4_offset] & 1);

    uint64_t *pdp = create_phys_addr_mapping(pml4[pml4_offset] & 0x0000FFFFFFFFF000ULL);
    assert(pdp[pdp_offset] & 1);
    if (cpu_supports_1gb_pages() && (pdp[pdp_offset] & VM_HUGE)) {
        return (pdp[pdp_offset] & 0x0000FFFFFFE00000ULL) + (virt_addr & 0x1FFFFF);
    }

    uint64_t *pd = create_phys_addr_mapping(pdp[pdp_offset] & 0x0000FFFFFFFFF000ULL);
    assert(pd[pd_offset] & 1);
    if (pd[pd_offset] & VM_HUGE) {
        return (pd[pd_offset] & 0x000FFFFC0000000ULL) + (virt_addr & 0x3FFFFFFF);
    }

    uint64_t *pt = create_phys_addr_mapping(pd[pd_offset] & 0x0000FFFFFFFFF000ULL);
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

    uint64_t *pml4 = create_phys_addr_mapping(get_cr3() & 0x0000FFFFFFFFF000ULL);
    uint64_t *pml4_entry = &pml4[pml4_offset];
    if (!(*pml4_entry & 1)) {
        return; // Page is already unmapped
    }

    uint64_t *pdp = create_phys_addr_mapping(*pml4_entry & 0x0000FFFFFFFFF000ULL);
    uint64_t *pdp_entry = &pdp[pdp_offset];
    if (!(*pdp_entry & 1)) {
        return;
    }

    uint64_t *pd = create_phys_addr_mapping(pdp[pdp_offset] & 0x0000FFFFFFFFF000ULL);
    uint64_t *pd_entry = &pd[pd_offset];
    if (!(*pd_entry & 1)) {
        return;
    }

    uint64_t *pt = create_phys_addr_mapping(pd[pd_offset] & 0x0000FFFFFFFFF000ULL);
    uint64_t *pt_entry = &pt[pt_offset];
    if (!(*pt_entry & 1)) {
        return;
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
    }

    if (all_empty(pd)) {
        if (free_phys_structure) {
            free_phys_page(get_phys_addr((uintptr_t) pd), process);
        }
        pdp[pdp_offset] = 0;
    }

    if (all_empty(pdp)) {
        if (free_phys_structure) {
            free_phys_page(get_phys_addr((uintptr_t) pdp), process);
        }
        pml4[pml4_offset] = 0;
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

    uint64_t *pml4 = create_phys_addr_mapping(get_cr3() & 0x0000FFFFFFFFF000ULL);
    uint64_t *pml4_entry = &pml4[pml4_offset];

    uint64_t *pdp = create_phys_addr_mapping(pml4[pml4_offset] & 0x0000FFFFFFFFF000ULL);
    if (!(*pml4_entry & 1)) {
        *pml4_entry = get_next_phys_page(process) | VM_WRITE | (VM_USER & flags) | 0x01;
        pdp = create_phys_addr_mapping(*pml4_entry & 0x0000FFFFFFFFF000ULL);
        memset(pdp, 0, PAGE_SIZE);
    }
    uint64_t *pdp_entry = &pdp[pdp_offset];

    uint64_t *pd = create_phys_addr_mapping(pdp[pdp_offset] & 0x0000FFFFFFFFF000ULL);
    if (!(*pdp_entry & 1)) {
        *pdp_entry = get_next_phys_page(process) | VM_WRITE | (VM_USER & flags) | 0x01;
        pd = create_phys_addr_mapping(pdp[pdp_offset] & 0x0000FFFFFFFFF000ULL);
        memset(pd, 0, PAGE_SIZE);
    }
    uint64_t *pd_entry = &pd[pd_offset];

    uint64_t *pt = create_phys_addr_mapping(pd[pd_offset] & 0x0000FFFFFFFFF000ULL);
    if (!(*pd_entry & 1)) {
        *pd_entry = get_next_phys_page(process) | VM_WRITE | (VM_USER & flags) | 0x01;
        pt = create_phys_addr_mapping(pd[pd_offset] & 0x0000FFFFFFFFF000ULL);
        memset(pt, 0, PAGE_SIZE);
    }
    uint64_t *pt_entry = &pt[pt_offset];

    *pt_entry = phys_addr | flags;
    do_tlb_flush(virt_addr);
}

void map_page_flags(uintptr_t virt_addr, uint64_t flags) {
    uint64_t pml4_offset = (virt_addr >> 39) & 0x1FF;
    uint64_t pdp_offset = (virt_addr >> 30) & 0x1FF;
    uint64_t pd_offset = (virt_addr >> 21) & 0x1FF;
    uint64_t pt_offset = (virt_addr >> 12) & 0x1FF;

    uint64_t *pml4 = create_phys_addr_mapping(get_cr3() & 0x0000FFFFFFFFF000ULL);
    uint64_t *pml4_entry = &pml4[pml4_offset];
    if (!(*pml4_entry & 1)) {
        return; // Page is isn't mapped, so will be mapped later on page fault and we can ignore it for now
    }

    uint64_t *pdp = create_phys_addr_mapping(*pml4_entry & 0x0000FFFFFFFFF000ULL);
    uint64_t *pdp_entry = &pdp[pdp_offset];
    if (!(*pdp_entry & 1)) {
        return;
    }

    uint64_t *pd = create_phys_addr_mapping(pdp[pdp_offset] & 0x0000FFFFFFFFF000ULL);
    uint64_t *pd_entry = &pd[pd_offset];
    if (!(*pd_entry & 1)) {
        return;
    }

    uint64_t *pt = create_phys_addr_mapping(pd[pd_offset] & 0x0000FFFFFFFFF000ULL);
    uint64_t *pt_entry = &pt[pt_offset];
    if (!(*pt_entry & 1)) {
        return;
    }

    flags |= (flags & VM_PROT_NONE ? 0x01 : 0);
    flags &= (VM_WRITE | VM_USER | VM_GLOBAL | VM_NO_EXEC | VM_COW | VM_SHARED | VM_PROT_NONE);
    *pt_entry &= ~0x8000000000000FFFULL;
    *pt_entry |= flags;
    flush_tlb(virt_addr);
}

uint64_t *get_page_table_entry(uintptr_t virt_addr) {
    virt_addr &= ~0xFFF;

    uint64_t pml4_offset = (virt_addr >> 39) & 0x1FF;
    uint64_t pdp_offset = (virt_addr >> 30) & 0x1FF;
    uint64_t pd_offset = (virt_addr >> 21) & 0x1FF;
    uint64_t pt_offset = (virt_addr >> 12) & 0x1FF;

    uint64_t *pml4 = create_phys_addr_mapping(get_cr3() & 0x0000FFFFFFFFF000ULL);
    uint64_t *pml4_entry = &pml4[pml4_offset];
    if (!(*pml4_entry & 1)) {
        return NULL;
    }

    uint64_t *pdp = create_phys_addr_mapping(*pml4_entry & 0x0000FFFFFFFFF000ULL);
    uint64_t *pdp_entry = &pdp[pdp_offset];
    if (!(*pdp_entry & 1)) {
        return NULL;
    }

    uint64_t *pd = create_phys_addr_mapping(pdp[pdp_offset] & 0x0000FFFFFFFFF000ULL);
    uint64_t *pd_entry = &pd[pd_offset];
    if (!(*pd_entry & 1)) {
        return NULL;
    }

    uint64_t *pt = create_phys_addr_mapping(pd[pd_offset] & 0x0000FFFFFFFFF000ULL);
    return &pt[pt_offset];
}

bool is_virt_addr_cow(uintptr_t virt_addr) {
    uint64_t *pt_entry = get_page_table_entry(virt_addr);
    return pt_entry && (*pt_entry & VM_COW);
}

void clear_initial_page_mappings() {
    update_vga_buffer();

    uint64_t cr3 = get_cr3();
    initial_kernel_process.arch_process.cr3 = cr3;
    idle_kernel_process.arch_process.cr3 = cr3;

    uint64_t *pml4 = create_phys_addr_mapping(cr3 & 0x0000FFFFFFFFF000ULL);
    pml4[0] = 0;
    pml4[PML4_RECURSIVE_INDEX] = 0;

    uintptr_t kernel_vm_end = ALIGN_UP(KERNEL_VM_END, PAGE_SIZE);
    uintptr_t kernel_vm_mapping_end = ALIGN_UP(kernel_vm_end, 2 * 1024 * 1024);
    for (uintptr_t i = kernel_vm_end; i < kernel_vm_mapping_end; i += PAGE_SIZE) {
        do_unmap_page(i, false, true, false, &idle_kernel_process);
    }
    load_cr3(cr3);
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

uintptr_t create_clone_process_paging_structure(struct process *process) {
    uint64_t pml4_addr = get_next_phys_page(process);
    uint64_t *pml4 = create_phys_addr_mapping(pml4_addr);
    uint64_t *old_pml4 = create_phys_addr_mapping(get_cr3() & 0x0000FFFFFFFFF000ULL);

    // Only clone the kernel entries in this table.
    memset(pml4, 0, (MAX_PML4_ENTRIES - 2) * sizeof(uint64_t));
    pml4[MAX_PML4_ENTRIES - 2] = old_pml4[MAX_PML4_ENTRIES - 2];
    pml4[MAX_PML4_ENTRIES - 1] = old_pml4[MAX_PML4_ENTRIES - 1];

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
    return pml4_addr;
}

void create_phys_id_map() {
    debug_log("Mapping physical address identity map: [ %#lX ]\n", g_phys_page_stats.phys_memory_max);
    uintptr_t stride = 2 * 1024 * 1024;
    if (cpu_supports_1gb_pages()) {
        stride *= 512;
    }

    for (uintptr_t phys_addr = 0; phys_addr < g_phys_page_stats.phys_memory_max; phys_addr += stride) {
        uintptr_t virt_addr = PHYS_ID_START + phys_addr;

        uint64_t pml4_offset = (virt_addr >> 39) & 0x1FF;
        uint64_t pdp_offset = (virt_addr >> 30) & 0x1FF;
        uint64_t pd_offset = (virt_addr >> 21) & 0x1FF;

        // The startup assembly code ensures that this recursive mapping is made. clear_initial_page_mappings() will remove it later,
        // since it is uneeded once the physical address mapping is created.
        uint64_t *pml4_entry = RECURSIVE_PML4_BASE + pml4_offset;
        uint64_t *pdp_entry = RECURSIVE_PDP_BASE + (0x1000 * pml4_offset) / sizeof(uint64_t) + pdp_offset;
        uint64_t *pd_entry = RECURSIVE_PD_BASE + (0x200000 * pml4_offset + 0x1000 * pdp_offset) / sizeof(uint64_t) + pd_offset;

        uint64_t mapping_flags = VM_WRITE | VM_GLOBAL | VM_NO_EXEC | 0x01;
        if (!(*pml4_entry & 1)) {
            *pml4_entry = get_next_phys_page(&idle_kernel_process) | mapping_flags;
            invlpg((uintptr_t) pdp_entry);
            memset(pdp_entry - pdp_offset, 0, PAGE_SIZE);
        }

        if (cpu_supports_1gb_pages()) {
            *pdp_entry = phys_addr | mapping_flags | VM_HUGE;
            continue;
        }

        if (!(*pdp_entry & 1)) {
            *pdp_entry = get_next_phys_page(&idle_kernel_process) | mapping_flags;
            invlpg((uintptr_t) pd_entry);
            memset(pd_entry - pd_offset, 0, PAGE_SIZE);
        }

        *pd_entry = phys_addr | mapping_flags | VM_HUGE;
    }

    // Flush thte entire TLB to ensure the new mappings take effect.
    load_cr3(get_cr3());
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
        flush_tlb(addr);
    }
}

void remove_paging_structure(uintptr_t phys_addr, struct vm_region *list) {
    // Disable interrupts since we have to change the value of CR3. This could potentially be avoided by
    // freeing the memory in old CR3 by traversing the physical addresses directly instead of using a
    // recursive page mapping.
    uint64_t interrupts_save = disable_interrupts_save();

    uint64_t old_cr3 = get_cr3();
    if (old_cr3 == phys_addr) {
        old_cr3 = idle_kernel_process.arch_process.cr3;
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
