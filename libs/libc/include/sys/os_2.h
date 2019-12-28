#ifndef _SYS_OS_2_H
#define _SYS_OS_2_H 1

#define MUTEX_AQUIRE           1
#define MUTEX_RELEASE          2
#define MUTEX_WAKE_AND_SET     3
#define MUTEX_RELEASE_AND_WAIT 4

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
};

struct initial_process_info {
    void *tls_start;
    unsigned long tls_size;
    unsigned long tls_alignment;
    void *stack_start;
    unsigned long stack_size;
    unsigned long guard_size;
    int main_tid;
};

int create_task(struct create_task_args *create_task_args);
void exit_task(void) __attribute__((__noreturn__));
int get_initial_process_info(struct initial_process_info *info);
int os_mutex(int *__protected, int op, int expected, int to_place, int to_wake, int *to_wait);
int set_thread_self_pointer(void *p);
int tgkill(int tgid, int tid, int signum);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SYS_OS_2_H */