#include <errno.h>
#include <sys/os_2.h>
#include <sys/syscall.h>

int enable_profiling(pid_t pid) {
    int ret = (int) syscall(SYS_enable_profiling, pid);
    __SYSCALL_TO_ERRNO(ret);
}
