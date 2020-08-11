#include <errno.h>
#include <sys/syscall.h>
#include <time.h>

int clock_gettime(clockid_t id, struct timespec *tp) {
    int ret = (int) syscall(SYS_CLOCK_GETTIME, id, tp);
    __SYSCALL_TO_ERRNO(ret);
}
