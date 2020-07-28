#ifndef _SYS_OS_2_H
#define _SYS_OS_2_H 1

#include <bits/__locked_robust_mutex_node.h>
#include <bits/clockid_t.h>
#include <stdint.h>
#include <sys/types.h>
#include <time.h>

#define MUTEX_AQUIRE           1
#define MUTEX_WAKE_AND_SET     3
#define MUTEX_RELEASE_AND_WAIT 4
#define MUTEX_OWNER_DIED       0x40000000U
#define MUTEX_WAITERS          0x80000000U

#define ROBUST_MUTEX_IS_VALID_IF_VALUE 1

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
    unsigned long program_entry;
    unsigned long program_dynamic_start;
    unsigned long program_dynamic_size;
    unsigned long program_offset;
    unsigned long program_size;
    unsigned long loader_dynamic_start;
    unsigned long loader_dynamic_size;
    unsigned long loader_offset;
    unsigned long loader_size;
    int has_interpreter;
};

#ifndef SYS_OS_2_NO_FUNCTIONS
int create_task(struct create_task_args *create_task_args);
void exit_task(void) __attribute__((__noreturn__));
int os_mutex(unsigned int *__protected, int op, int expected, int to_place, int to_wake, unsigned int *to_wait);
int set_thread_self_pointer(void *p, struct __locked_robust_mutex_node **list_head);
int tgkill(int tgid, int tid, int signum);
int getcpuclockid(int tgid, int tid, clockid_t *clock_id);
#endif /* SYS_OS_2_NO_FUNCTIONS */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SYS_OS_2_H */
