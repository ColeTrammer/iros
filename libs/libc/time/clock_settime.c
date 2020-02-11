#include <errno.h>
#include <sys/syscall.h>
#include <time.h>

int clock_settime(clockid_t id, const struct timespec *tp) {
    int ret = (int) syscall(SC_CLOCK_SETTIME, id, tp);
    __SYSCALL_TO_ERRNO(ret);
}