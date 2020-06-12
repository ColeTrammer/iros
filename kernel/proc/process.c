#include <assert.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/fs/procfs.h>
#include <kernel/fs/vfs.h>
#include <kernel/hal/output.h>
#include <kernel/mem/anon_vm_object.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/mem/vm_region.h>
#include <kernel/proc/process.h>
#include <kernel/proc/task.h>
#include <kernel/sched/task_sched.h>
#include <kernel/time/clock.h>
#include <kernel/time/timer.h>
#include <kernel/util/hash_map.h>

// #define PROC_REF_COUNT_DEBUG
// #define PROCESSES_DEBUG

extern struct process initial_kernel_process;

static struct hash_map *map;

HASH_DEFINE_FUNCTIONS(process, struct process, pid_t, pid)

void proc_drop_process(struct process *process, pid_t tid, bool free_paging_structure) {
    // Reassign the main tid of the process if it exits early
    spin_lock(&process->lock);
    if (process->main_tid == tid) {
        struct task *new_task = find_task_for_process(process->pid);
        process->main_tid = new_task ? new_task->tid : -1;
        assert(process->main_tid != tid);
    }
    spin_unlock(&process->lock);

    int fetched_ref_count = atomic_fetch_sub(&process->ref_count, 1);

#ifdef PROC_REF_COUNT_DEBUG
    debug_log("Process ref count: [ %d, %d ]\n", process->pid, fetched_ref_count - 1);
#endif /* PROC_REF_COUNT_DEBUG */

    assert(fetched_ref_count > 0);
    if (fetched_ref_count == 1) {
#ifdef PROCESSES_DEBUG
        debug_log("Destroying process: [ %d ]\n", process->pid);
#endif /* PROCESSES_DEBUG */
        hash_del(map, &process->pid);
        procfs_unregister_process(process);

        proc_kill_arch_process(process, free_paging_structure);
#ifdef PROCESSES_DEBUG
        debug_log("Destroyed arch process: [ %d ]\n", process->pid);
#endif /* PROCESSES_DEBUG */

        struct vm_region *region = process->process_memory;
        while (region != NULL) {
            if (region->vm_object) {
                drop_vm_object(region->vm_object);
            }

            region = region->next;
        }

        region = process->process_memory;
        while (region != NULL) {
            struct vm_region *temp = region->next;
            free(region);
            region = temp;
        }

        for (size_t i = 0; i < FOPEN_MAX; i++) {
            if (process->files[i].file != NULL) {
                fs_close(process->files[i].file);
                process->files[i].file = NULL;
            }
        }

        struct user_mutex *user_mutex = process->used_user_mutexes;
        while (user_mutex != NULL) {
            struct user_mutex *next = user_mutex->next;
            free(user_mutex);
            user_mutex = next;
        }

        struct timer *timer = process->timers;
        while (timer) {
            debug_log("Destroying timer: [ %p ]\n", timer);
            struct timer *next = timer->proc_next;
            time_delete_timer(timer);
            timer = next;
        }

        free(process->supplemental_gids);

#ifdef PROC_REF_COUNT_DEBUG
        debug_log("Finished destroying process: [ %d ]\n", process->pid);
#endif /* PROC_REF_COUNT_DEBUG */
        if (process->cwd) {
            drop_tnode(process->cwd);
        }

        if (process->exe) {
            drop_tnode(process->exe);
        }

        if (process->args_context) {
            free_program_args(process->args_context);
        }

        time_destroy_clock(process->process_clock);
        free(process->name);
        free(process);
        return;
    }
}

void proc_add_process(struct process *process) {
#ifdef PROC_REF_COUNT_DEBUG
    debug_log("Process ref count: [ %d, %d ]\n", process->pid, 1);
#endif /* PROC_REF_COUNT_DEBUG */
#ifdef PROCESSES_DEBUG
    debug_log("Adding process: [ %d ]\n", process->pid);
#endif /* PROCESSES_DEBUG */
    process->ref_count = 1;
    hash_put(map, process);

    procfs_register_process(process);
}

void proc_bump_process(struct process *process) {
    int fetched_ref_count = atomic_fetch_add(&process->ref_count, 1);
    (void) fetched_ref_count;
#ifdef PROC_REF_COUNT_DEBUG
    debug_log("Process ref count: [ %d, %d ]\n", process->pid, fetched_ref_count + 1);
#endif /* PROC_REF_COUNT_DEBUG */
    assert(fetched_ref_count > 0);
}

uintptr_t proc_allocate_user_stack(struct process *process) {
    // Guard Pages: 0xFFFFFE7FFFDFE000 - 0xFFFFFE7FFFDFF000
    // Stack Pages: 0xFFFFFE7FFFDFF000 - 0xFFFFFE7FFFFFF000

    struct vm_object *stack_object = vm_create_anon_object(PAGE_SIZE + 2 * 1024 * 1024);
    assert(stack_object);

    struct vm_region *task_stack = calloc(1, sizeof(struct vm_region));
    task_stack->flags = VM_USER | VM_WRITE | VM_NO_EXEC | VM_STACK;
    task_stack->type = VM_TASK_STACK;
    task_stack->start = find_first_kernel_vm_region()->start - PAGE_SIZE - 2 * 1024 * 1024;
    task_stack->end = task_stack->start + 2 * 1024 * 1024;
    task_stack->vm_object = stack_object;
    task_stack->vm_object_offset = PAGE_SIZE;
    process->process_memory = add_vm_region(process->process_memory, task_stack);

    struct vm_region *guard_page = calloc(1, sizeof(struct vm_region));
    guard_page->flags = VM_PROT_NONE | VM_NO_EXEC;
    guard_page->type = VM_TASK_STACK_GUARD;
    guard_page->start = task_stack->start - PAGE_SIZE;
    guard_page->end = guard_page->start + PAGE_SIZE;
    guard_page->vm_object = bump_vm_object(stack_object);
    guard_page->vm_object_offset = 0;
    process->process_memory = add_vm_region(process->process_memory, guard_page);

    return task_stack->end;
}

struct process *find_by_pid(pid_t pid) {
    return hash_get(map, &pid);
}

void proc_set_sig_pending(struct process *process, int n) {
    task_set_sig_pending(find_task_for_process(process->pid), n);
}

bool proc_in_group(struct process *process, gid_t group) {
    if (!process->supplemental_gids) {
        return false;
    }

    for (size_t i = 0; i < process->supplemental_gids_size; i++) {
        if (process->supplemental_gids[i] == group) {
            return true;
        }
    }

    return false;
}

void init_processes() {
    debug_log("Initializing processes\n");
    map = hash_create_hash_map(process_hash, process_equals, process_key);
    assert(map);
    proc_add_process(&initial_kernel_process);
    initial_kernel_process.name = strdup("init");
}
