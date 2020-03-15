#ifndef _SYS_OS_2_H
#define _SYS_OS_2_H 1

#include <bits/__locked_robust_mutex_node.h>
#include <bits/clockid_t.h>
#include <stdint.h>
#include <sys/types.h>

#define MUTEX_AQUIRE           1
#define MUTEX_WAKE_AND_SET     3
#define MUTEX_RELEASE_AND_WAIT 4
#define MUTEX_OWNER_DIED       0x40000000U
#define MUTEX_WAITERS          0x80000000U

#define ROBUST_MUTEX_IS_VALID_IF_VALUE 1

#define READ_PROCFS_SCHED 1

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct create_task_args {
    unsigned long entry;
    unsigned long stack_start;
    void *arg;
    unsigned long push_onto_stack;
    int *tid_ptr;
    void *thread_self_pointer;
    struct __locked_robust_mutex_node **locked_robust_mutex_list_head;
};

struct initial_process_info {
    void *tls_start;
    unsigned long tls_size;
    unsigned long tls_alignment;
    void *stack_start;
    unsigned long stack_size;
    unsigned long guard_size;
    int main_tid;
    int isatty_mask;
};

struct proc_info {
    char name[64];
    pid_t pid;
    char state[64];
    uid_t uid;
    gid_t gid;
    pid_t ppid;
    mode_t umask;
    uid_t euid;
    gid_t egid;
    pid_t pgid;
    pid_t sid;
    char tty[64];
    int priority;
    int nice;
    size_t virtual_memory;
    size_t resident_memory;
    uint64_t user_ticks;
    uint64_t kernel_ticks;
};

struct proc_global_info {
    uint64_t idle_ticks;
    uint64_t user_ticks;
    uint64_t kernel_ticks;
};

int create_task(struct create_task_args *create_task_args);
void exit_task(void) __attribute__((__noreturn__));
int get_initial_process_info(struct initial_process_info *info);
int os_mutex(unsigned int *__protected, int op, int expected, int to_place, int to_wake, unsigned int *to_wait);
int set_thread_self_pointer(void *p, struct __locked_robust_mutex_node **list_head);
int tgkill(int tgid, int tid, int signum);
int getcpuclockid(int tgid, int tid, clockid_t *clock_id);

int read_procfs_info(struct proc_info **info, size_t *length, int flags);
void free_procfs_info(struct proc_info *info);

int read_procfs_global_info(struct proc_global_info *info);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SYS_OS_2_H */