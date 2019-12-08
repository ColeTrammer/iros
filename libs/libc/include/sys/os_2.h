#ifndef _SYS_OS_2_H
#define _SYS_OS_2_H 1

#define MUTEX_AQUIRE           1
#define MUTEX_RELEASE          2
#define MUTEX_WAKE_AND_SET     3
#define MUTEX_RELEASE_AND_WAIT 4

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

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SYS_OS_2_H */