#include <sys/os_2.h>
#include <sys/syscall.h>

int set_thread_self_pointer(void *p, struct __locked_robust_mutex_node **list_head) {
    return syscall(SYS_SET_THREAD_SELF_POINTER, p, list_head);
}
