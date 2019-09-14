#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

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

/* Copying args and envp is necessary because they could be saved on the program stack we are about to overwrite */
uintptr_t map_program_args(uintptr_t start, char **argv, char **envp) {
    debug_log("Mapping Program Args: [ %#.16lX ]\n", (uintptr_t) start);

    size_t argc = 0;
    size_t args_str_length = 0;
    while (argv[argc++] != NULL) {
        args_str_length += strlen(argv[argc - 1]) + 1;
    }

    size_t envc = 0;
    size_t env_str_length = 0;
    while (envp[envc++] != NULL) {
        env_str_length += strlen(envp[envc - 1]) + 1;
    }

    char **args_copy = calloc(argc, sizeof(char**));
    char **envp_copy = calloc(envc, sizeof(char**));

    char *args_buffer = malloc(args_str_length);
    char *env_buffer = malloc(env_str_length);

    ssize_t j = 0;
    ssize_t i = 0;
    while (argv[i] != NULL) {
        ssize_t last = j;
        while (argv[i][j - last] != '\0') {
            args_buffer[j] = argv[i][j - last];
            j++;
        }
        args_buffer[j++] = '\0';
        args_copy[i++] = args_buffer + last;
    }
    args_copy[i] = NULL;

    j = 0;
    i = 0;
    while (envp[i] != NULL) {
        ssize_t last = j;
        while (envp[i][j - last] != '\0') {
            env_buffer[j] = envp[i][j - last];
            j++;
        }
        env_buffer[j++] = '\0';
        envp_copy[i++] = env_buffer + last;
    }
    envp_copy[i] = NULL;

    char **argv_start = (char**) (start - sizeof(char**));

    size_t count = argc + envc;
    char *args_start = (char*) (argv_start - count);

    for (i = 0; args_copy[i] != NULL; i++) {
        args_start -= strlen(args_copy[i]) + 1;
        strcpy(args_start, args_copy[i]);
        argv_start[i - argc] = args_start;
    }

    argv_start[0] = NULL;

    for (i = 0; envp_copy[i] != NULL; i++) {
        args_start -= strlen(envp_copy[i]) + 1;
        strcpy(args_start, envp_copy[i]);
        argv_start[i - count] = args_start;
    }

    argv_start[-(argc + 1)] = NULL;

    args_start = (char*) ((((uintptr_t) args_start) & ~0x7) - 0x08);

    args_start -= sizeof(size_t);
    *((size_t*) args_start) = argc - 1;
    args_start -= sizeof(char**);
    *((char***) args_start) = argv_start - argc;
    args_start -= sizeof(char**);
    *((char***) args_start) = argv_start - count;

    free(args_copy);
    free(envp_copy);
    free(args_buffer);
    free(env_buffer);

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
    int error;
    struct file *program = fs_open(file_name, &error);
    assert(program != NULL && error == 0);

    fs_seek(program, 0, SEEK_END);
    long length = fs_tell(program);
    fs_seek(program, 0, SEEK_SET);

    void *buffer = malloc(length);
    fs_read(program, buffer, length);

    fs_close(program);

    assert(elf64_is_valid(buffer));

    struct process *process = calloc(1, sizeof(struct process));
    process->pid = get_next_pid();
    process->process_memory = NULL;
    process->kernel_process = false;
    process->sched_state = READY;
    process->next = NULL;

    uintptr_t old_paging_structure = get_current_paging_structure();
    uintptr_t structure = create_paging_structure(process->process_memory, false);
    load_paging_structure(structure);

    elf64_load_program(buffer, length, process);
    elf64_map_heap(buffer, process);

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

    arch_run_process(process);
}

struct process *get_current_process() {
    return current_process;
}

/* Must be called from unpremptable context */
void free_process(struct process *process, bool free_paging_structure, bool __free_pid) {
    arch_free_process(process, free_paging_structure);

    if (__free_pid) {
        free_pid(process->pid);
    }

    struct vm_region *region = process->process_memory;
    while (region != NULL) {
        struct vm_region *temp = region->next;
        free(region);
        region = temp;
    }

    free(process);
}