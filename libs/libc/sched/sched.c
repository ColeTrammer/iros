#include <errno.h>
#include <sys/syscall.h>

int sched_yield(void) {
    int ret = (int) syscall(SC_YIELD);
    __SYSCALL_TO_ERRNO(ret);
}