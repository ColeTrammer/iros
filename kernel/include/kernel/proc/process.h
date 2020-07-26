#ifndef _KERNEL_PROC_PROCESS_H
#define _KERNEL_PROC_PROCESS_H 1

#include <signal.h>
#include <stdio.h>
#include <sys/times.h>
#include <sys/types.h>

#include <kernel/proc/user_mutex.h>
#include <kernel/util/mutex.h>
#include <kernel/util/spinlock.h>

// clang-format off
#include <kernel/arch/arch.h>
#include ARCH_SPECIFIC(proc/arch_process.h)
// clang-format on

struct args_context;
struct clock;
struct file;
struct initial_process_info;
struct timer;
struct tnode;

struct file_descriptor {
    struct file *file;
    int fd_flags;
};

struct process {
    struct tnode *cwd;
    struct tnode *exe;
    struct file_descriptor files[FOPEN_MAX];

    struct vm_region *process_memory;

    struct user_mutex *used_user_mutexes;
    spinlock_t user_mutex_lock;

    struct clock *process_clock;
    struct timer *timers;
    struct timer *alarm_timer;

    struct args_context *args_context;

    struct task *task_list;
    pid_t main_tid;

    pid_t pid;
    pid_t pgid;
    pid_t ppid;
    pid_t sid;

    uid_t uid;
    uid_t euid;
    gid_t gid;
    gid_t egid;

    gid_t *supplemental_gids;
    size_t supplemental_gids_size;

    mode_t umask;

    int tty;
    int ref_count;

    char *name;
    size_t resident_memory;

    struct arch_process arch_process;

    struct tms times;

    struct timespec start_time;

    bool should_trace;

    struct sigaction sig_state[_NSIG];
    stack_t alt_stack;
    mutex_t lock;
};

void proc_drop_process(struct process *process, struct task *task, bool free_paging_structure);
void proc_add_process(struct process *process);
void proc_bump_process(struct process *process);
uintptr_t proc_allocate_user_stack(struct process *process, struct initial_process_info *info);
struct process *find_by_pid(pid_t pid);
void proc_set_sig_pending(struct process *process, int n);
void proc_for_each_with_pgid(pid_t pgid, void (*callback)(struct process *process, void *closure), void *closure);

int proc_getgroups(size_t size, gid_t *list);
int proc_setgroups(size_t size, const gid_t *list);
bool proc_in_group(struct process *process, gid_t group);

void init_processes();

#endif /* _KERNEL_PROC_PROCESS_H */
