#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <kernel/fs/vfs.h>
#include <kernel/mem/page.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/proc/process.h>
#include <kernel/proc/elf64.h>
#include <kernel/proc/pid.h>
#include <kernel/sched/process_sched.h>
#include <kernel/hal/output.h>

static struct process *current_process;
static struct process initial_kernel_process;

/* Needs Fix: Can Only Map Args and Envp If They Do Not Overlap, Which They Do When Passing Defualt ENVP */
uintptr_t map_program_args(uintptr_t start, char **argv, char **envp) {
    char **argv_start = (char**) (start - sizeof(char**));

    debug_log("Mapping Program Args: [ %#.16lX ]\n", (uintptr_t) start);

    size_t argc = 0;
    while (argv[argc++] != NULL);

    size_t envc = 0;
    while (envp[envc++] != NULL);

    size_t count = argc + envc;

    char *args_start = (char*) (argv_start - count);

    ssize_t i;
    for (i = 0; argv[i] != NULL; i++) {
        args_start -= strlen(argv[i]) + 1;
        strcpy(args_start, argv[i]);
        argv_start[i - argc] = args_start;
    }

    argv_start[0] = NULL;

    for (i = 0; envp[i] != NULL; i++) {
        args_start -= strlen(envp[i]) + 1;
        strcpy(args_start, envp[i]);
        argv_start[i - count] = args_start;
    }

    argv_start[-(argc + 1)] = NULL;

    args_start -= sizeof(size_t);
    *((size_t*) args_start) = argc - 1;
    args_start -= sizeof(char**);
    *((char***) args_start) = argv_start - argc;
    args_start -= sizeof(char**);
    *((char***) args_start) = argv_start - count;

    return (uintptr_t) args_start;
}

void init_kernel_process() {
    current_process = &initial_kernel_process;

    arch_init_kernel_process(current_process);

    initial_kernel_process.kernel_process = true;
    initial_kernel_process.pid = 0;
    initial_kernel_process.sched_state = RUNNING;
    initial_kernel_process.next = NULL;

    sched_add_process(&initial_kernel_process);
}

struct process *load_process(const char *file_name) {
    struct file *program = fs_open(file_name);
    fs_seek(program, 0, SEEK_END);
    long length = fs_tell(program);
    fs_seek(program, 0, SEEK_SET);

    void *buffer = malloc(length);
    fs_read(program, buffer, length);

    fs_close(program);

    struct process *process = calloc(1, sizeof(struct process));
    process->pid = get_next_pid();
    process->process_memory = NULL;
    process->kernel_process = false;
    process->sched_state = READY;
    process->next = NULL;

    uintptr_t old_paging_structure = get_current_paging_structure();
    uintptr_t structure = create_paging_structure(process->process_memory, false);
    load_paging_structure(structure);

    uint64_t start = elf64_get_start(buffer);
    uint64_t size = elf64_get_size(buffer);
    uint64_t num_pages = NUM_PAGES(start, start + size);
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
    free(buffer);

    load_paging_structure(old_paging_structure);

    debug_log("Loaded Process: [ %d, %s ]\n", process->pid, file_name);
    return process;
}

/* Must be called from unpremptable context */
void run_process(struct process *process) {
    if (current_process->sched_state == RUNNING) {
        current_process->sched_state = READY;
    }
    current_process = process;
    current_process->sched_state = RUNNING;

    if (process->pid != 0) {
        debug_log("Proceeding to Run Process: [ %d ]\n", process->pid);
    }

    arch_run_process(process);
}

struct process *get_current_process() {
    return current_process;
}

/* Must be called from unpremptable context */
void free_process(struct process *process) {
    arch_free_process(process);

    free_pid(process->pid);

    struct vm_region *region = process->process_memory;
    while (region != NULL) {
        struct vm_region *temp = region->next;
        free(region);
        region = temp;
    }

    free(process);
}