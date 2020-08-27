#include <errno.h>
#include <sys/syscall.h>

int sched_yield(void) {
    int ret = (int) syscall(SYS_SCHED_YIELD);
    __SYSCALL_TO_ERRNO(ret);
}
