#include <errno.h>
#include <sys/syscall.h>
#include <time.h>

int clock_nanosleep(clockid_t id, int flags, const struct timespec *amount, struct timespec *remaining) {
    int ret = (int) syscall(SC_CLOCK_NANOSLEEP, id, flags, amount, remaining);
    __SYSCALL_TO_ERRNO(ret);
}
