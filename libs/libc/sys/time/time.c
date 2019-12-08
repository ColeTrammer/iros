#include <errno.h>
#include <sys/syscall.h>
#include <sys/time.h>

int gettimeofday(struct timeval *__restrict tv, void *__restrict tz) {
    int ret = (int) syscall(SC_GETTIMEOFDAY, tv, tz);
    __SYSCALL_TO_ERRNO(ret);
}