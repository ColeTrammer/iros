#include <errno.h>
#include <sys/syscall.h>
#include <time.h>

int clock_getres(clockid_t id, struct timespec *res) {
    int ret = (int) syscall(SYS_clock_getres, id, res);
    __SYSCALL_TO_ERRNO(ret);
}
