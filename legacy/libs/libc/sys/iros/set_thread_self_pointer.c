#include <sys/iros.h>
#include <sys/syscall.h>

int set_thread_self_pointer(void *p, struct __locked_robust_mutex_node **list_head) {
    return syscall(SYS_set_thread_self_pointer, p, list_head);
}
