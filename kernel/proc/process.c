#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <kernel/fs/fs_manager.h>
#include <kernel/mem/page.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/proc/process.h>
#include <kernel/proc/elf64.h>
#include <kernel/proc/pid.h>

static struct process *current_process;
static struct process initial_kernel_process;

void init_kernel_process() {
    current_process = &initial_kernel_process;

    arch_init_kernel_process(current_process);

    initial_kernel_process.kernel_process = true;
    initial_kernel_process.pid = 0;
    initial_kernel_process.next = NULL;
}

struct process *load_process(const char *file_name) {
    VFILE *program = fs_open(file_name);
    fs_seek(program, 0, SEEK_END);
    long length = fs_tell(program);
    fs_seek(program, 0, SEEK_SET);

    void *buffer = malloc(length);
    fs_read(program, buffer, length);

    struct process *process = calloc(1, sizeof(struct process));
    process->pid = get_next_pid();
    process->process_memory = clone_kernel_vm();
    process->kernel_process = false;
    process->next = NULL;

    uintptr_t structure = create_paging_structure(process->process_memory, false);
    load_paging_structure(structure);

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
        process->process_memory = add_vm_region(process->process_memory, region);
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
    process->process_memory = add_vm_region(process->process_memory, process_heap);

    struct vm_region *process_stack = calloc(1, sizeof(struct vm_region));
    process_stack->flags = VM_USER | VM_WRITE | VM_NO_EXEC;
    process_stack->type = VM_PROCESS_STACK;
    process_stack->start = find_vm_region(VM_KERNEL_TEXT)->start - 2 * PAGE_SIZE;
    process_stack->end = process_stack->start + PAGE_SIZE;
    process->process_memory = add_vm_region(process->process_memory, process_stack);
    map_vm_region(process_stack);

    arch_load_process(process, elf64_get_entry(buffer));

    return process;
}

void run_process(struct process *process) {
    current_process = process;
    arch_run_process(process);
}

struct process *get_current_process() {
    return current_process;
}