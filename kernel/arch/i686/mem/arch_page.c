#include <string.h>
#include <sys/param.h>

#include <kernel/arch/i686/asm_utils.h>
#include <kernel/hal/processor.h>
#include <kernel/hal/x86/drivers/vga.h>
#include <kernel/mem/kernel_vm.h>
#include <kernel/mem/page.h>
#include <kernel/mem/page_frame_allocator.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/proc/process.h>

void flush_tlb(uintptr_t addr) {
    invlpg(addr);
    broadcast_flush_tlb(addr, 1);
}

uintptr_t get_phys_addr(uintptr_t virt_addr) {
    uint32_t pd_offset = (virt_addr >> 22) & 0x3FF;
    uint32_t pt_offset = (virt_addr >> 12) & 0x3FF;

    uint32_t *pd = create_temp_phys_addr_mapping(get_cr3() & ~0xFFF);
    uint32_t pd_entry = pd[pd_offset];
    free_temp_phys_addr_mapping(pd);

    assert(pd_entry & 1);
    uint32_t *pt = create_temp_phys_addr_mapping(pd_entry & ~0xFFF);
    uint32_t pt_entry = pt[pt_offset];
    free_temp_phys_addr_mapping(pt);

    assert(pt_entry & 1);
    return (pt_entry & ~0x3FF) + (virt_addr & 0xFFF);
}

static bool all_empty(uint32_t *page) {
    for (size_t i = 0; i < PAGE_SIZE / sizeof(uint32_t); i++) {
        if (page[i] & 1) {
            return false;
        }
    }
    return true;
}

void do_unmap_page(uintptr_t virt_addr, bool free_phys, bool free_phys_structure, bool broadcast_tlb_flush, struct process *process) {
    void (*do_tlb_flush)(uintptr_t) = broadcast_tlb_flush ? &flush_tlb : &invlpg;

    uint32_t pd_offset = (virt_addr >> 22) & 0x3FF;
    uint32_t pt_offset = (virt_addr >> 12) & 0x3FF;

    uint32_t *pd = create_temp_phys_addr_mapping(get_cr3() & ~0xFFF);
    uint32_t *pd_entry = &pd[pd_offset];
    if (!(*pd_entry & 1)) {
        free_temp_phys_addr_mapping(pd);
        return;
    }

    uint32_t *pt = create_temp_phys_addr_mapping(*pd_entry & ~0xFFF);
    uint32_t *pt_entry = &pt[pt_offset];
    if (!(*pt_entry & 1)) {
        free_temp_phys_addr_mapping(pd);
        free_temp_phys_addr_mapping(pt);
        return;
    }

    if (free_phys) {
        free_phys_page(*pt_entry & ~0xFFF, process);
    }
    *pt_entry = 0;
    do_tlb_flush(virt_addr);

    if (all_empty(pt)) {
        if (free_phys_structure) {
            free_phys_page(*pd_entry & ~0xFFF, process);
        }
        pd[pd_offset] = 0;
    }

    free_temp_phys_addr_mapping(pt);
    free_temp_phys_addr_mapping(pd);
}

void do_map_phys_page(uintptr_t phys_addr, uintptr_t virt_addr, uint64_t flags, bool broadcast_flush_tlb, struct process *process) {
    void (*do_tlb_flush)(uintptr_t) = broadcast_flush_tlb ? &flush_tlb : &invlpg;

    flags &= (VM_WRITE | VM_USER | VM_GLOBAL | VM_NO_EXEC | VM_COW | VM_SHARED | VM_PROT_NONE);
    flags |= 0x01;

    uint32_t pd_offset = (virt_addr >> 22) & 0x3FF;
    uint32_t pt_offset = (virt_addr >> 12) & 0x3FF;

    bool zero_pt = false;

    uint32_t *pd = create_temp_phys_addr_mapping(get_cr3() & ~0xFFF);
    uint32_t *pd_entry = &pd[pd_offset];
    if (!(*pd_entry & 1)) {
        *pd_entry = get_next_phys_page(process) | VM_WRITE | (VM_USER & flags) | 0x01;
        zero_pt = true;
    }
    uint32_t pt_mapping = *pd_entry;
    free_temp_phys_addr_mapping(pd);

    uint32_t *pt = create_temp_phys_addr_mapping(pt_mapping & ~0xFFF);
    if (zero_pt) {
        memset(pt, 0, PAGE_SIZE);
    }

    uint32_t *pt_entry = &pt[pt_offset];

    *pt_entry = phys_addr | flags;
    do_tlb_flush(virt_addr);

    free_temp_phys_addr_mapping(pt);
}

void map_page_flags(uintptr_t virt_addr, uint64_t flags64) {
    // NOTE: this explicitly ignores the VM_NO_EXEC bit, which requires PAE.
    uint32_t flags = flags64;

    uint32_t pd_offset = (virt_addr >> 22) & 0x3FF;
    uint32_t pt_offset = (virt_addr >> 12) & 0x3FF;

    uint32_t *pd = create_temp_phys_addr_mapping(get_cr3() & ~0xFFF);
    uint32_t pd_entry = pd[pd_offset];
    free_temp_phys_addr_mapping(pd);

    if (!(pd_entry & 1)) {
        return;
    }

    uint32_t *pt = create_temp_phys_addr_mapping(pd_entry & ~0xFFF);
    uint32_t *pt_entry = &pt[pt_offset];
    if (!(*pt_entry & 1)) {
        free_temp_phys_addr_mapping(pt);
        return;
    }

    flags |= (flags & VM_PROT_NONE ? 0x01 : 0);
    flags &= (VM_WRITE | VM_USER | VM_GLOBAL | VM_NO_EXEC | VM_COW | VM_SHARED | VM_PROT_NONE);
    *pt_entry &= ~0xFFF;
    *pt_entry |= flags;
    free_temp_phys_addr_mapping(pt);

    flush_tlb(virt_addr);
}

bool is_virt_addr_cow(uintptr_t virt_addr) {
    uint32_t pd_offset = (virt_addr >> 22) & 0x3FF;
    uint32_t pt_offset = (virt_addr >> 12) & 0x3FF;

    uint32_t *pd = create_temp_phys_addr_mapping(get_cr3() & ~0xFFF);
    uint32_t pd_entry = pd[pd_offset];
    free_temp_phys_addr_mapping(pd);

    if (!(pd_entry & 1)) {
        return false;
    }

    uint32_t *pt = create_temp_phys_addr_mapping(pd_entry & ~0xFFF);
    uint32_t pt_entry = pt[pt_offset];
    free_temp_phys_addr_mapping(pt);

    return !!(pt_entry & VM_COW);
}

static void mark_virt_addr_as_cow(uintptr_t virt_addr) {
    uint32_t pd_offset = (virt_addr >> 22) & 0x3FF;
    uint32_t pt_offset = (virt_addr >> 12) & 0x3FF;

    uint32_t *pd = create_temp_phys_addr_mapping(get_cr3() & ~0xFFF);
    uint32_t pd_entry = pd[pd_offset];
    free_temp_phys_addr_mapping(pd);

    if (!(pd_entry & 1)) {
        return;
    }

    uint32_t *pt = create_temp_phys_addr_mapping(pd_entry & ~0xFFF);
    uint32_t *pt_entry = &pt[pt_offset];
    if (!(*pt_entry & 1)) {
        free_temp_phys_addr_mapping(pt);
        return;
    }

    *pt_entry |= VM_COW;
    *pt_entry &= ~VM_WRITE;
    free_temp_phys_addr_mapping(pt);

    flush_tlb(virt_addr);
}

void mark_region_as_cow(struct vm_region *region) {
    for (uintptr_t addr = region->start; addr < region->end; addr += PAGE_SIZE) {
        mark_virt_addr_as_cow(addr);
    }
}

void clear_initial_page_mappings() {
    update_vga_buffer();

    uint32_t cr3 = get_cr3();
    initial_kernel_process.arch_process.cr3 = cr3;
    idle_kernel_process.arch_process.cr3 = cr3;

    {
        // NOTE: cr3 must be identity mapped because we haven't cleared the mapping
        //       yet, so its safe to cast the physical address directly to a virtual
        //       address.
        uint32_t *pd = (void *) (cr3 & ~0xFFF);
        pd[0] = 0;
    }

    uintptr_t kernel_vm_end = ALIGN_UP(KERNEL_VM_END, PAGE_SIZE);
    uintptr_t kernel_vm_mapping_end = ALIGN_UP(kernel_vm_end, 2 * 1024 * 1024);
    for (uintptr_t i = kernel_vm_end; i < kernel_vm_mapping_end; i += PAGE_SIZE) {
        do_unmap_page(i, false, true, false, &idle_kernel_process);
    }

    load_cr3(cr3);
}

void remove_paging_structure(uintptr_t phys_addr, struct vm_region *list) {
    // Disable interrupts since we have to change the value of CR3. This could potentially be avoided by
    // freeing the memory in old CR3 by traversing the physical addresses directly.
    uint64_t save = disable_interrupts_save();

    uint64_t old_cr3 = get_cr3();
    if (old_cr3 == phys_addr) {
        old_cr3 = idle_kernel_process.arch_process.cr3;
    } else {
        load_cr3(phys_addr);
    }

    soft_remove_paging_structure(list);

    load_cr3(old_cr3);
    free_phys_page(phys_addr, NULL);

    interrupts_restore(save);
}
