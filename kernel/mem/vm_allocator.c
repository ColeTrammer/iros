#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <kernel/display/terminal.h>
#include <kernel/display/vga.h>
#include <kernel/mem/kernel_vm.h>
#include <kernel/mem/page.h>
#include <kernel/mem/vm_region.h>
#include <kernel/mem/vm_allocator.h>

static struct vm_region *kernel_vm_list;
static struct vm_region kernel;
static struct vm_region kernel_heap;
static struct vm_region vga_buffer;
static uint64_t kernel_heap_start;

void init_vm_allocator(uint64_t kernel_phys_start, uint64_t kernel_phys_end) {
    kernel.start = KERNEL_VM_START & ~0xFFF;
    kernel.end = ((KERNEL_VM_START + kernel_phys_end - kernel_phys_start) & ~0xFFF) + PAGE_SIZE;
    kernel.flags = VM_READ | VM_WRITE;
    kernel_vm_list = add_vm_region(kernel_vm_list, &kernel);

    vga_buffer.start = kernel.end;
    vga_buffer.end = kernel.end + PAGE_SIZE;
    vga_buffer.flags = VM_READ | VM_WRITE | VM_NO_EXEC;
    kernel_vm_list = add_vm_region(kernel_vm_list, &vga_buffer);
    map_phys_page(VGA_PHYS_ADDR, vga_buffer.start);
    set_vga_buffer((void*) vga_buffer.start);

    kernel_heap.start = vga_buffer.start + PAGE_SIZE;
    kernel_heap.end = kernel_heap.start;
    kernel_heap.flags = VM_READ | VM_WRITE | VM_NO_EXEC;
    kernel_vm_list = add_vm_region(kernel_vm_list, &kernel_heap);

    clear_initial_page_mappings();
}

void *add_vm_pages(size_t n) {
    uint64_t old_end = get_vm_region(kernel_vm_list, kernel_heap_start)->end;
    if (extend_vm_region(kernel_vm_list, kernel_heap_start, n) == -1) {
        return NULL; // indicate there is no room
    }
    for (size_t i = 0; i < n; i++) {
        map_page(old_end + i * PAGE_SIZE);
    }
    return (void*) old_end;
}

void remove_vm_pages(size_t n) {
    uint64_t old_end = get_vm_region(kernel_vm_list, kernel_heap_start)->end;
    if (contract_vm_region(kernel_vm_list, kernel_heap_start, n) == -1) {
        printf("%s\n", "Error: Removed to much memory");
        abort();
    }
    for (size_t i = 1; i <= n; i++) {
        unmap_page(old_end - i * PAGE_SIZE);
    }
}