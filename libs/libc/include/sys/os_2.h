#ifndef _SYS_OS_2_H
#define _SYS_OS_2_H 1

#define MUTEX_AQUIRE  1
#define MUTEX_RELEASE 2

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int create_task(unsigned long rip, unsigned long rsp, void *arg, unsigned long push_onto_stack, int *tid_ptr);
void exit_task(void) __attribute__((__noreturn__));
int gettid(void);
int os_mutex(int *__protected, int operation, int expected, int to_place);
int tgkill(int tgid, int tid, int sig);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SYS_OS_2_H */