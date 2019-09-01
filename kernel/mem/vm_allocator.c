#include <sys/types.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <kernel/hal/x86_64/drivers/vga.h>
#include <kernel/mem/kernel_vm.h>
#include <kernel/mem/page.h>
#include <kernel/mem/vm_region.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/proc/process.h>
#include <kernel/hal/output.h>

static struct vm_region kernel_text;
static struct vm_region kernel_rod;
static struct vm_region kernel_data;
static struct vm_region kernel_heap;
static struct vm_region vga_buffer;
static struct vm_region initrd;

void init_vm_allocator(uintptr_t initrd_phys_start, uintptr_t initrd_phys_end) {
    kernel_text.start = KERNEL_VM_START & ~0xFFF;
    kernel_text.end = kernel_text.start + NUM_PAGES(KERNEL_TEXT_START, KERNEL_TEXT_END) * PAGE_SIZE;
    kernel_text.flags = VM_GLOBAL;
    kernel_text.type = VM_KERNEL_TEXT;
    get_current_process()->process_memory = add_vm_region(get_current_process()->process_memory, &kernel_text);

    kernel_rod.start = kernel_text.end;
    kernel_rod.end = kernel_rod.start + NUM_PAGES(KERNEL_ROD_START, KERNEL_ROD_END) * PAGE_SIZE;
    kernel_rod.flags = VM_GLOBAL | VM_NO_EXEC;
    kernel_rod.type = VM_KERNEL_ROD;
    get_current_process()->process_memory = add_vm_region(get_current_process()->process_memory, &kernel_rod);

    kernel_data.start = kernel_rod.end;
    kernel_data.end = kernel_data.start + NUM_PAGES(KERNEL_DATA_START, KERNEL_BSS_END) * PAGE_SIZE;
    kernel_data.flags = VM_WRITE | VM_GLOBAL | VM_NO_EXEC;
    kernel_data.type = VM_KERNEL_DATA;
    get_current_process()->process_memory = add_vm_region(get_current_process()->process_memory, &kernel_data);

    vga_buffer.start = kernel_data.end;
    vga_buffer.end = vga_buffer.start + PAGE_SIZE;
    vga_buffer.flags = VM_WRITE | VM_GLOBAL | VM_NO_EXEC;
    vga_buffer.type = VM_VGA;
    get_current_process()->process_memory = add_vm_region(get_current_process()->process_memory, &vga_buffer);
    map_phys_page(VGA_PHYS_ADDR, vga_buffer.start, vga_buffer.flags);
    set_vga_buffer((void*) vga_buffer.start);

    initrd.start = vga_buffer.end;
    initrd.end = ((initrd.start + initrd_phys_end - initrd_phys_start) & ~0xFFF) + PAGE_SIZE;
    initrd.flags = VM_GLOBAL | VM_NO_EXEC;
    initrd.type = VM_INITRD;
    get_current_process()->process_memory = add_vm_region(get_current_process()->process_memory, &initrd);
    for (int i = 0; initrd.start + i < initrd.end; i += PAGE_SIZE) {
        map_phys_page(initrd_phys_start + i, initrd.start + i, initrd.flags);
    }

    kernel_heap.start = initrd.end;
    kernel_heap.end = kernel_heap.start;
    kernel_heap.flags = VM_WRITE | VM_GLOBAL | VM_NO_EXEC;
    kernel_heap.type = VM_KERNEL_HEAP;
    get_current_process()->process_memory = add_vm_region(get_current_process()->process_memory, &kernel_heap);

    clear_initial_page_mappings();

    uintptr_t new_structure = create_paging_structure(get_current_process()->process_memory, true);
    load_paging_structure(new_structure);

    debug_log("Finished Initializing VM Allocator\n");
}

void *add_vm_pages_end(size_t n, uint64_t type) {
    struct vm_region *region = get_vm_region(get_current_process()->process_memory, type);
    uintptr_t old_end = region->end;
    if (extend_vm_region_end(get_current_process()->process_memory, VM_KERNEL_HEAP, n) < 0) {
        return NULL; // indicate there is no room
    }
    for (size_t i = 0; i < n; i++) {
        map_page(old_end + i * PAGE_SIZE, region->flags);
    }
    
    memset((void*) old_end, 0, n * PAGE_SIZE);
    return (void*) old_end;
}

void *add_vm_pages_start(size_t n, uint64_t type) {
    struct vm_region *region = get_vm_region(get_current_process()->process_memory, type);
    uintptr_t old_start = region->start;
    if (extend_vm_region_start(get_current_process()->process_memory, VM_KERNEL_HEAP, n) < 0) {
        return NULL; // indicate there is no room
    }
    for (size_t i = 1; i <= n; i++) {
        map_page(old_start - i * PAGE_SIZE, region->flags);
    }
    
    memset((void*) (old_start - n * PAGE_SIZE), 0, n * PAGE_SIZE);
    return (void*) old_start;
}

void remove_vm_pages_end(size_t n, uint64_t type) {
    uintptr_t old_end = get_vm_region(get_current_process()->process_memory, type)->end;
    if (contract_vm_region_end(get_current_process()->process_memory, type, n) < 0) {
        printf("%s\n", "Error: Removed to much memory");
        abort();
    }
    for (size_t i = 1; i <= n; i++) {
        unmap_page(old_end - i * PAGE_SIZE);
    }
}

void remove_vm_pages_start(size_t n, uint64_t type) {
    uintptr_t old_start = get_vm_region(get_current_process()->process_memory, type)->start;
    if (contract_vm_region_start(get_current_process()->process_memory, type, n) < 0) {
        printf("%s\n", "Error: Removed to much memory");
        abort();
    }
    for (size_t i = 0; i < n; i++) {
        unmap_page(old_start + i * PAGE_SIZE);
    }
}

struct vm_region *find_vm_region(uint64_t type) {
    return get_vm_region(get_current_process()->process_memory, type);
}

struct vm_region *clone_kernel_vm() {
    if (get_current_process()->process_memory == NULL) { return NULL; }

    struct vm_region *list = NULL;
    struct vm_region *iter = get_current_process()->process_memory;
    while (iter != NULL) {
        if (iter->flags & VM_GLOBAL) {
            struct vm_region *to_add = malloc(sizeof(struct vm_region));
            memcpy(to_add, iter, sizeof(struct vm_region));
            list = add_vm_region(list, to_add);
        }

        iter = iter->next;
    }

    return list;
}