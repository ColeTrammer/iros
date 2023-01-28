#include <kernel/hal/output.h>
#include <kernel/mem/page.h>
#include <kernel/mem/page_frame_allocator.h>
#include <kernel/mem/vm_allocator.h>

// WARNING: this function assumes the vm_region list is currently
//          mapped into memory.
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

void map_page(uintptr_t virt_addr, uint64_t flags, struct process *process) {
    do_map_phys_page(get_next_phys_page(process), virt_addr, flags, true, process);
}

void map_phys_page(uintptr_t phys_addr, uintptr_t virt_addr, uint64_t flags, struct process *process) {
    do_map_phys_page(phys_addr, virt_addr, flags, true, process);
}

void unmap_page(uintptr_t virt_addr, struct process *process) {
    do_unmap_page(virt_addr, true, true, true, process);
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
