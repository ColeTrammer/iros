#ifndef _KERNEL_PROC_TASK_H
#define _KERNEL_PROC_TASK_H 1

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/times.h>
#include <sys/types.h>

#include <kernel/fs/file.h>
#include <kernel/mem/vm_region.h>
#include <kernel/proc/process.h>

// clang-format off
#include <kernel/arch/arch.h>
#include ARCH_SPECIFIC(proc/task.h)
// clang-format on

enum sched_state { RUNNING, READY, WAITING, EXITING };

struct task {
    struct arch_task arch_task;

    struct task *next;
    struct task *prev;

    enum sched_state sched_state;

    sigset_t sig_mask;
    sigset_t saved_sig_mask;
    sigset_t sig_pending;

    int tid;

    bool kernel_task : 1;
    bool in_kernel : 1;
    bool in_sigsuspend : 1;
    bool can_send_self_signals : 1;
    bool sleeping : 1;

    time_t sleep_end;

    struct process *process;

    struct arch_fpu_state fpu;
};

void init_kernel_task();
void arch_init_kernel_task(struct task *kernel_task);

struct task *load_kernel_task(uintptr_t entry);
void arch_load_kernel_task(struct task *task, uintptr_t entry);

struct task *load_task(const char *file_name);
void arch_load_task(struct task *task, uintptr_t entry);

void run_task(struct task *task);
void arch_run_task(struct task *task);

void free_task(struct task *task, bool free_paging_structure);
void arch_free_task(struct task *task, bool free_paging_structure);

struct task *get_current_task();

uintptr_t map_program_args(uintptr_t start, char **argv, char **envp);

int get_next_tid();

void task_set_sig_pending(struct task *task, int signum);
void task_unset_sig_pending(struct task *task, int signum);
int task_get_next_sig(struct task *task);
void task_do_sig(struct task *task, int signum);
void task_do_sig_handler(struct task *task, int signum);
bool task_is_sig_blocked(struct task *task, int signum);
void proc_notify_parent(pid_t child_pid);

bool task_in_kernel(struct task *task);

#endif /* _KERNEL_PROC_TASK_H */