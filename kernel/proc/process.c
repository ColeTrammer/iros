#include <assert.h>
#include <stdlib.h>

#include <kernel/fs/vfs.h>
#include <kernel/hal/output.h>
#include <kernel/mem/vm_region.h>
#include <kernel/proc/process.h>
#include <kernel/proc/task.h>
#include <kernel/sched/task_sched.h>
#include <kernel/util/hash_map.h>

// #define PROC_REF_COUNT_DEBUG
// #define PROCESSES_DEBUG

extern struct process initial_kernel_process;

static struct hash_map *map;

static int hash(void *pid, int num_buckets) {
    assert(pid);
    return *((pid_t *) pid) % num_buckets;
}

static int equals(void *p1, void *p2) {
    assert(p1);
    assert(p2);
    return *((pid_t *) p1) == *((pid_t *) p2);
}

static void *key(void *p) {
    assert(p);
    return &((struct process *) p)->pid;
}

void proc_drop_process_unlocked(struct process *process, bool free_paging_structure) {
#ifdef PROC_REF_COUNT_DEBUG
    debug_log("Process ref count: [ %d, %d ]\n", process->pid, process->ref_count - 1);
#endif /* PROC_REF_COUNT_DEBUG */

    assert(process->ref_count > 0);
    process->ref_count--;
    if (process->ref_count == 0) {
#ifdef PROCESSES_DEBUG
        debug_log("Destroying process: [ %d ]\n", process->pid);
#endif /* PROCESSES_DEBUG */
        hash_del(map, &process->pid);
        spin_unlock(&process->lock);

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

#ifdef PROC_REF_COUNT_DEBUG
        debug_log("Finished destroying process: [ %d ]\n", process->pid);
#endif /* PROC_REF_COUNT_DEBUG */
        if (process->cwd) {
            free(process->cwd->name);
            free(process->cwd);
        }
        free(process);
        return;
    }

    spin_unlock(&process->lock);
}

void proc_drop_process(struct process *process, bool free_paging_structure) {
    spin_lock(&process->lock);
    proc_drop_process_unlocked(process, free_paging_structure);
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
}

void proc_bump_process(struct process *process) {
    spin_lock(&process->lock);
#ifdef PROC_REF_COUNT_DEBUG
    debug_log("Process ref count: [ %d, %d ]\n", process->pid, process->ref_count + 1);
#endif /* PROC_REF_COUNT_DEBUG */
    assert(process->ref_count > 0);
    process->ref_count++;
    spin_unlock(&process->lock);
}

struct process *find_by_pid(pid_t pid) {
    return hash_get(map, &pid);
}

void proc_set_sig_pending(struct process *process, int n) {
    task_set_sig_pending(find_task_for_process(process->pid), n);
}

void init_processes() {
    debug_log("Initializing processes\n");
    map = hash_create_hash_map(hash, equals, key);
    assert(map);
    proc_add_process(&initial_kernel_process);
}