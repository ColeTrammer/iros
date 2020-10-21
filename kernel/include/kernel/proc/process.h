#ifndef _KERNEL_PROC_PROCESS_H
#define _KERNEL_PROC_PROCESS_H 1

#include <signal.h>
#include <stdio.h>
#include <sys/resource.h>
#include <sys/times.h>
#include <sys/types.h>

#include <kernel/proc/user_mutex.h>
#include <kernel/proc/wait_queue.h>
#include <kernel/util/hash_map.h>
#include <kernel/util/list.h>
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
struct queued_signal;
struct timer;
struct tnode;

#define PROCESS_MAX_PRIORITY     39
#define PROCESS_DEFAULT_PRIORITY 20

struct file_descriptor {
    struct file *file;
    int fd_flags;
};

enum process_state { PS_NONE, PS_STOPPED, PS_CONTINUED, PS_TERMINATED };

struct process {
    struct process *sibling_next;
    struct process *sibling_prev;
    struct process *children;
    // This lock protects the children pointer of the current process and the
    // sibling fields of all the process's children.
    spinlock_t children_lock;

    // Serialized by the spinlock to ensure that the parent pointer is always valid.
    // Accessing the parent requires calling proc_get_parent(), and must be ended with
    // a call to proc_drop_parent().
    struct process *parent;
    spinlock_t parent_lock;

    struct tnode *cwd;
    struct tnode *exe;
    struct file_descriptor files[FOPEN_MAX];

    struct vm_region *process_memory;

    struct user_mutex *used_user_mutexes;
    spinlock_t user_mutex_lock;

    struct wait_queue one_task_left_queue;
    struct wait_queue child_wait_queue;

    struct hash_entry hash;

    struct clock *process_clock;
    struct list_node timer_list;
    struct timer *alarm_timer;
    struct timer *profile_timer;
    struct timer *virtual_timer;

    struct args_context *args_context;

    struct list_node task_list;
    pid_t main_tid;

    pid_t pid;
    pid_t pgid;
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
    int priority;

    char *name;
    size_t resident_memory;

    struct arch_process arch_process;

    struct timespec start_time;

    struct rusage rusage_self;
    struct rusage rusage_children;

    struct rlimit limits[RLIMIT_NLIMITS];

    int should_profile;
    struct vm_region *profile_buffer;
    size_t profile_buffer_size;
    spinlock_t profile_buffer_lock;

    bool should_trace : 1;
    bool zombie : 1;
    bool in_execve : 1;
    bool terminated_bc_signal : 1;

    enum process_state state;
    union {
        int exit_code;
        int terminating_signal;
        int stop_signal;
    };
    struct queued_signal *signal_for_parent;

    struct sigaction sig_state[_NSIG];
    stack_t alt_stack;
    mutex_t lock;
};

struct process *get_current_process(void);

void proc_reset_for_execve(struct process *process);
void proc_drop_process(struct process *process, struct task *task, bool free_paging_structure);
void proc_add_process(struct process *process);
struct process *proc_bump_process(struct process *process);
uintptr_t proc_allocate_user_stack(struct process *process, struct initial_process_info *info);
struct process *find_by_pid(pid_t pid);
void proc_set_sig_pending(struct process *process, int n);
void proc_for_each_with_pgid(pid_t pgid, void (*callback)(struct process *process, void *closure), void *closure);
void proc_for_each_with_euid(uid_t euid, void (*callback)(struct process *process, void *closure), void *closure);

int proc_getrusage(int who, struct rusage *rusage);
int proc_getrlimit(struct process *process, int what, struct rlimit *limit);
int proc_setrlimit(struct process *process, int what, const struct rlimit *limit);
int proc_nice(int inc);
int proc_getpriority(int which, id_t who);
int proc_setpriority(int which, id_t who, int value);

int proc_getgroups(size_t size, gid_t *list);
int proc_setgroups(size_t size, const gid_t *list);
bool proc_in_group(struct process *process, gid_t group);

void proc_add_child(struct process *parent, struct process *child);
int proc_get_waitable_process(struct process *parent, pid_t wait_spec, struct process **process);
void proc_consume_wait_info(struct process *parent, struct process *child, enum process_state state);
void proc_set_process_state(struct process *process, enum process_state state, int info, bool terminated_bc_signal);

void init_processes();

static inline struct process *proc_get_parent(struct process *process) {
    spin_lock(&process->parent_lock);
    struct process *parent = proc_bump_process(process->parent);
    spin_unlock(&process->parent_lock);
    return parent;
}

static inline void proc_drop_parent(struct process *parent) {
    proc_drop_process(parent, NULL, false);
}

extern struct process initial_kernel_process;
extern struct process idle_kernel_process;

#endif /* _KERNEL_PROC_PROCESS_H */
