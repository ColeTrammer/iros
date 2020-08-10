#include <errno.h>
#include <stdatomic.h>
#include <string.h>
#include <sys/param.h>

#include <kernel/mem/vm_allocator.h>
#include <kernel/proc/process.h>
#include <kernel/proc/profile.h>
#include <kernel/sched/task_sched.h>

// Must be called with process->profile_lock held.
void proc_write_profile_buffer(struct process *process, const void *buffer, size_t sz) {
    sz = MIN(sz, process->profile_buffer->end - process->profile_buffer->start - process->profile_buffer_size);
    memcpy((void *) process->profile_buffer->start + process->profile_buffer_size, buffer, sz);
    process->profile_buffer_size += sz;
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
    }
    mutex_unlock(&process->lock);

    return 0;
}
