#include <errno.h>
#include <sys/syscall.h>

int sched_yield(void) {
    int ret = (int) syscall(SYS_sched_yield);
    __SYSCALL_TO_ERRNO(ret);
}
