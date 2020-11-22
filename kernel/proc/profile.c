#include <errno.h>
#include <stdatomic.h>
#include <string.h>
#include <sys/param.h>

#include <kernel/fs/inode.h>
#include <kernel/hal/hw_timer.h>
#include <kernel/mem/inode_vm_object.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/proc/profile.h>
#include <kernel/proc/task.h>
#include <kernel/sched/task_sched.h>

// Must be called with process->profile_lock held.
void proc_write_profile_buffer(struct process *process, const void *buffer, size_t sz) {
    sz = MIN(sz, process->profile_buffer->end - process->profile_buffer->start - process->profile_buffer_size);
    memcpy((void *) process->profile_buffer->start + process->profile_buffer_size, buffer, sz);
    process->profile_buffer_size += sz;
}

void proc_record_memory_map(struct process *process) {
    spin_lock(&process->profile_buffer_lock);
    char raw_buffer[sizeof(struct profile_event_memory_map) + PROFILE_MAX_MEMORY_MAP * sizeof(struct profile_event_memory_object)];
    struct profile_event_memory_map *pev = (void *) raw_buffer;
    pev->type = PEV_MEMORY_MAP;
    pev->count = 0;

    for (struct vm_region *region = process->process_memory; region && pev->count < PROFILE_MAX_MEMORY_MAP; region = region->next) {
        if (region->vm_object && region->vm_object->type == VM_INODE && !(region->flags & VM_NO_EXEC)) {
            struct profile_event_memory_object *obj = &pev->mem[pev->count++];
            obj->start = region->start;
            obj->end = region->end;
            struct inode *inode = ((struct inode_vm_object_data *) region->vm_object->private_data)->inode;
            obj->inode_id = inode->index;
            obj->fs_id = inode->fsid;
        }
    }

    proc_write_profile_buffer(process, raw_buffer, PEV_MEMORY_MAP_SIZE(pev));
    spin_unlock(&process->profile_buffer_lock);
}

static int num_profiling;

static void on_hw_profile_tick(struct hw_timer_channel *channel, struct irq_context *context) {
    (void) channel;

    struct task *current = get_current_task();
    if (atomic_load(&current->process->should_profile)) {
        // To seriously support profiling multiple threads, the buffer and lock should be per-thread and not per-process.
        if (spin_trylock(&current->process->profile_buffer_lock)) {
            // Make sure not to write into a stale buffer.
            if (current->process->profile_buffer) {
                proc_record_profile_stack(context->task_state);
            }
            spin_unlock(&current->process->profile_buffer_lock);
        }
    }
}

void proc_maybe_start_profile_timer(void) {
    if (atomic_fetch_add(&num_profiling, 1) == 0) {
        struct hw_timer *timer = hw_profile_timer();
        timer->ops->setup_interval_timer(timer, 0, 1000, IRQ_HANDLER_ALL_CPUS | IRQ_HANDLER_REQUEST_NMI, on_hw_profile_tick);
    }
}

void proc_maybe_stop_profile_timer(void) {
    if (atomic_fetch_sub(&num_profiling, 1) == 1) {
        struct hw_timer *timer = hw_profile_timer();
        timer->ops->disable_channel(timer, 0);
    }
}

int proc_enable_profiling(pid_t pid) {
    struct process *process = find_by_pid(pid);
    if (!process) {
        return -ESRCH;
    }

    struct process *current = get_current_process();
    if (current->euid != process->uid && current->euid != 0) {
        return -EPERM;
    }

    mutex_lock(&process->lock);
    if (!process->should_profile) {
        struct vm_region *profile_buffer = vm_allocate_kernel_region(PROFILE_BUFFER_MAX);
        if (!profile_buffer) {
            mutex_unlock(&process->lock);
            return -ENOMEM;
        }

        process->profile_buffer = profile_buffer;
        atomic_store(&process->should_profile, 1);
        proc_record_memory_map(process);

        proc_maybe_start_profile_timer();
    }
    mutex_unlock(&process->lock);

    return 0;
}

ssize_t proc_read_profile(pid_t pid, void *buffer, size_t size) {
    struct process *process = find_by_pid(pid);
    if (!process) {
        return -ESRCH;
    }

    struct process *current = get_current_process();
    if (current->euid != process->uid && current->euid != 0) {
        return -EPERM;
    }

    ssize_t ret = 0;
    mutex_lock(&process->lock);
    int should_profile = process->should_profile;
    if (!should_profile) {
        ret = -EINVAL;
    } else {
        spin_lock(&process->profile_buffer_lock);
        ret = MIN(size, process->profile_buffer_size);
        memcpy(buffer, (void *) process->profile_buffer->start, ret);
        process->profile_buffer_size = 0;
        spin_unlock(&process->profile_buffer_lock);
    }
    mutex_unlock(&process->lock);
    return ret;
}

int proc_disable_profiling(pid_t pid) {
    struct process *process = find_by_pid(pid);
    if (!process) {
        return -ESRCH;
    }

    struct process *current = get_current_process();
    if (current->euid != process->uid && current->euid != 0) {
        return -EPERM;
    }

    mutex_lock(&process->lock);
    if (process->should_profile) {
        spin_lock(&process->profile_buffer_lock);
        vm_free_kernel_region(process->profile_buffer);
        process->profile_buffer = NULL;
        process->profile_buffer_size = 0;
        atomic_store(&process->should_profile, 0);
        spin_unlock(&process->profile_buffer_lock);

        proc_maybe_stop_profile_timer();
    }
    mutex_unlock(&process->lock);

    return 0;
}
