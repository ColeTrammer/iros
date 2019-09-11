#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <assert.h>

#include <kernel/mem/vm_allocator.h>
#include <kernel/proc/process.h>
#include <kernel/proc/pid.h>
#include <kernel/sched/process_sched.h>
#include <kernel/fs/vfs.h>

#include <kernel/hal/hal.h>
#include <kernel/hal/output.h>
#include <kernel/arch/x86_64/proc/process.h>

void arch_sys_print(struct process_state *process_state) {
    screen_print((char*) process_state->cpu_state.rsi, process_state->cpu_state.rdx);

    process_state->cpu_state.rax = true;
}

void arch_sys_exit(struct process_state *process_state) {
    disable_interrupts();

    struct process *process = get_current_process();
    process->sched_state = EXITING;

    int exit_code = (int) process_state->cpu_state.rsi;
    debug_log("Process Exited: [ %d, %d ]\n", process->pid, exit_code);

    enable_interrupts();
    while (1);
}

void arch_sys_sbrk(struct process_state *process_state) {
    intptr_t increment = process_state->cpu_state.rsi;

    debug_log("SBRK Called: [ %#.16lX ]\n", increment);

    void *res;
    if (increment < 0) {
        res = add_vm_pages_end(0, VM_PROCESS_HEAP);
        remove_vm_pages_end(-increment, VM_PROCESS_HEAP);
    } else {
        res = add_vm_pages_end(increment, VM_PROCESS_HEAP);
    }
    process_state->cpu_state.rax = (uint64_t) res;
}

void arch_sys_fork(struct process_state *process_state) {
    struct process *child = calloc(1, sizeof(struct process));
    child->pid = get_next_pid();
    child->sched_state = READY;
    child->kernel_process = false;
    child->process_memory = clone_process_vm();

    memcpy(&child->arch_process.process_state, process_state, sizeof(struct process_state));
    child->arch_process.process_state.cpu_state.rax = 0;
    child->arch_process.cr3 = clone_process_paging_structure();
    child->arch_process.kernel_stack = KERNEL_PROC_STACK_START;
    child->arch_process.setup_kernel_stack = true;

    process_state->cpu_state.rax = child->pid;

    sched_add_process(child);
}

void arch_sys_open(struct process_state *process_state) {
    const char *path = (const char*) process_state->cpu_state.rsi;
    int flags = (int) process_state->cpu_state.rdx;
    mode_t mode = (mode_t) process_state->cpu_state.rcx;
    
    (void) flags;
    (void) mode;
    
    assert(path != NULL);

    struct process *process = get_current_process();
    struct file *file = fs_open(path);

    /* Start at 3 because 0,1,2 are reserved for stdin, stdio, and stderr */
    for (size_t i = 3; i < FOPEN_MAX; i++) {
        if (process->files[i] == NULL) {
            process->files[i] = file;
            process_state->cpu_state.rax = i;
            return;
        }
    }

    /* Max files allocated, should return some ERROR */
    assert(false);
}

void arch_sys_read(struct process_state *process_state)  {
    int fd = (int) process_state->cpu_state.rsi;
    void *buf = (void*) process_state->cpu_state.rdx;
    size_t count = (size_t) process_state->cpu_state.rcx;

    struct process *process = get_current_process();
    struct file *file = process->files[fd];
    assert(file != NULL);

    fs_read(file, buf, count);

    /* Should be checking for errors and bytes read in fs_read and returning them here */
    process_state->cpu_state.rax = count;
}

void arch_sys_write(struct process_state *process_state) {
    (void) process_state;

    assert(false);
}

void arch_sys_close(struct process_state *process_state) {
    int fd = (int) process_state->cpu_state.rsi;

    struct process *process = get_current_process();
    fs_close(process->files[fd]);
    process->files[fd] = NULL;

    /* Should be returning error codes here */
    process_state->cpu_state.rax = 0;
}