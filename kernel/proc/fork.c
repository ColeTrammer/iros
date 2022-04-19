#include <stdlib.h>

#include <kernel/fs/vfs.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/proc/pid.h>
#include <kernel/proc/process.h>
#include <kernel/proc/task.h>
#include <kernel/time/clock.h>
#include <kernel/time/timer.h>

// FORK_DEBUG

pid_t proc_fork(void) {
    struct task *parent = get_current_task();
    struct task *child = calloc(1, sizeof(struct task));
    struct process *child_process = calloc(1, sizeof(struct process));
    child->process = child_process;

    mutex_lock(&parent->process->lock);

    child->tid = get_next_tid();
    child_process->pid = get_next_pid();
    child_process->main_tid = child->tid;
    init_mutex(&child_process->lock);
    init_spinlock(&child_process->user_mutex_lock);
    init_spinlock(&child_process->children_lock);
    init_spinlock(&child_process->parent_lock);
    init_list(&child_process->task_list);
    init_list(&child_process->timer_list);
    init_spinlock(&child->sig_lock);
    init_spinlock(&child->unblock_lock);
    init_list(&child->queued_signals);
    init_wait_queue(&child_process->one_task_left_queue);
    init_wait_queue(&child_process->child_wait_queue);
    proc_add_process(child_process);
    child->sched_state = RUNNING_INTERRUPTIBLE;
    child->kernel_task = false;
    child_process->process_memory = clone_process_vm();
    child_process->tty = parent->process->tty;
    list_append(&child_process->task_list, &child->process_list);

#ifdef FORK_DEBUG
    debug_log("Forking Task: [ %d ]\n", parent->process->pid);
#endif /* FORK_DEBUG */

    memcpy(&child->arch_task.task_state, parent->user_task_state, sizeof(struct task_state));
    task_set_sys_call_return_value(&child->arch_task.task_state, 0);
    child_process->arch_process.cr3 = create_clone_process_paging_structure(child_process);
    child->kernel_stack = vm_allocate_kernel_region(KERNEL_STACK_SIZE);
    child->arch_task.user_thread_pointer = parent->arch_task.user_thread_pointer;
    child_process->cwd = bump_tnode(parent->process->cwd);
    child_process->pgid = parent->process->pgid;
    child->process->uid = parent->process->uid;
    child->process->euid = parent->process->euid;
    child->process->gid = parent->process->gid;
    child->process->egid = parent->process->egid;
    child->process->sid = parent->process->sid;
    child->process->umask = parent->process->umask;
    child_process->priority = parent->process->priority;
    child->process->parent = parent->process;
    proc_add_child(parent->process, child->process);
    proc_bump_process(child_process);
    child->process->start_time = time_read_clock(CLOCK_REALTIME);
    child->sig_pending = 0;
    child->sig_mask = parent->sig_mask;
    child_process->exe = bump_tnode(parent->process->exe);
    child_process->name = strdup(parent->process->name);
    memcpy(child_process->limits, parent->process->limits, sizeof(child_process->limits));
    memcpy(child_process->sig_state, parent->process->sig_state, sizeof(child_process->sig_state));
    child_process->process_clock = time_create_clock(CLOCK_PROCESS_CPUTIME_ID);
    child->task_clock = time_create_clock(CLOCK_THREAD_CPUTIME_ID);

    child_process->supplemental_gids_size = parent->process->supplemental_gids_size;
    child_process->supplemental_gids = malloc(parent->process->supplemental_gids_size * sizeof(gid_t));
    memcpy(child_process->supplemental_gids, parent->process->supplemental_gids, parent->process->supplemental_gids_size * sizeof(gid_t));

    task_align_fpu(child);
    memcpy(child->fpu.aligned_state, parent->fpu.aligned_state, FPU_IMAGE_SIZE);

    for (size_t i = 0; i < FOPEN_MAX; i++) {
        if (parent->process->files[i].file) {
            child_process->files[i] = fs_dup_accross_fork(parent->process->files[i]);
        }
    }

    mutex_unlock(&parent->process->lock);

    disable_interrupts();
    sched_add_task(child);
    return child_process->pid;
}
