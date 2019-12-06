#ifndef _SYS_OS_2_H
#define _SYS_OS_2_H 1

#define MUTEX_AQUIRE  1
#define MUTEX_RELEASE 2

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct initial_process_info {
    void *tls_start;
    unsigned long tls_size;
    unsigned long tls_alignment;
    void *stack_start;
    unsigned long stack_size;
    unsigned long guard_size;
    int main_tid;
};

int create_task(unsigned long rip, unsigned long rsp, void *arg, unsigned long push_onto_stack, int *tid_ptr, void *thread_self_pointer);
void exit_task() __attribute__((__noreturn__));
int os_mutex(int *__protected, int operation, int expected, int to_place);
int tgkill(int tgid, int tid, int sig);
int get_initial_process_info(struct initial_process_info *info);
int set_thread_self_pointer(void *p);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SYS_OS_2_H */