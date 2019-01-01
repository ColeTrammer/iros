#include <stddef.h>
#include <stdint.h>

#include <kernel/mem/kernel_vm.h>
#include <kernel/mem/page.h>
#include <kernel/mem/vm_region.h>
#include <kernel/mem/vm_allocator.h>

static struct vm_region *kernel_vm_list;
static struct vm_region kernel;
static struct vm_region kernel_heap;
static uint64_t kernel_heap_start;

void init_vm_allocator(uint64_t kernel_phys_start, uint64_t kernel_phys_end) {
    kernel.start = KERNEL_VMA & ~0xFFF;
    kernel.end = ((KERNEL_VMA + kernel_phys_end - kernel_phys_start) & ~0xFFF) + PAGE_SIZE;
    kernel.flags = VM_READ | VM_WRITE;
    kernel_vm_list = add_vm_region(kernel_vm_list, &kernel);

    kernel_heap_start = kernel.end;
    kernel_heap.start = kernel_heap_start;
    kernel_heap.end = kernel_heap.start;
    kernel_heap.flags = VM_READ | VM_WRITE | VM_NO_EXEC;
    kernel_vm_list = add_vm_region(kernel_vm_list, &kernel_heap);
}

void *alloc_pages(size_t n) {
    uint64_t old_end = get_vm_region(kernel_vm_list, kernel_heap_start)->end;
    if (!extend_vm_region(kernel_vm_list, kernel_heap_start, n)) {
        return NULL; // indicate there is no room
    }
    return (void*) old_end;
}