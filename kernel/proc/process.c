#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/fs/fs_manager.h>
#include <kernel/mem/page.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/proc/process.h>
#include <kernel/proc/elf64.h>

void load_process(const char *file_name) {
    VFILE *program = fs_open(file_name);
    fs_seek(program, 0, SEEK_END);
    long length = fs_tell(program);
    fs_seek(program, 0, SEEK_SET);

    void *buffer = malloc(length);
    fs_read(program, buffer, length);

    uint64_t start = elf64_get_start(buffer);
    uint64_t size = elf64_get_size(buffer);
    uint64_t num_pages = size / PAGE_SIZE;
    for (uint64_t i = 0; i < num_pages; i++) {
        map_page(start + i * PAGE_SIZE, VM_USER | VM_WRITE);
    }
    memcpy((void*) start, buffer, length);

    uint64_t types[] = { VM_PROCESS_TEXT, VM_PROCESS_ROD, VM_PROCESS_DATA, VM_PROCESS_BSS };
    for (size_t i = 0; i < 4; i++) {
        uint64_t type = types[i];
        struct vm_region *region = elf64_create_vm_region(buffer, type);
        add_vm(region);
        map_vm_region_flags(region);

        if (type == VM_PROCESS_BSS) {
            memset((void*) region->start, 0, region->end - region->start);
        }
    }

    struct vm_region *process_heap = calloc(1, sizeof(struct vm_region));
    process_heap->flags = VM_USER | VM_WRITE | VM_NO_EXEC;
    process_heap->type = VM_PROCESS_HEAP;
    process_heap->start = ((start + size) & ~0xFFF) + PAGE_SIZE;
    process_heap->end = process_heap->start;
    add_vm(process_heap);

    struct vm_region *process_stack = calloc(1, sizeof(struct vm_region));
    process_stack->flags = VM_USER | VM_WRITE | VM_NO_EXEC;
    process_stack->type = VM_PROCESS_STACK;
    process_stack->start = find_vm_region(VM_KERNEL_TEXT)->start - 2 * PAGE_SIZE;
    process_stack->end = process_stack->start + PAGE_SIZE;
    add_vm(process_stack);
    map_vm_region(process_stack);

    run_process(elf64_get_entry(buffer), process_stack->end);
}