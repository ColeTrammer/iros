#include <errno.h>
#include <sys/syscall.h>
#include <sys/time.h>

int gettimeofday(struct timeval *__restrict tv, void *__restrict tz) {
    int ret = (int) syscall(SC_GETTIMEOFDAY, tv, tz);
    __SYSCALL_TO_ERRNO(ret);
}

int utimes(const char *filename, const struct timeval times[2]) {
    int ret = (int) syscall(SC_UTIMES, filename, times);
    __SYSCALL_TO_ERRNO(ret);
}