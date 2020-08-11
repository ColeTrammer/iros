#include <errno.h>
#include <sys/syscall.h>
#include <time.h>

int timer_gettime(timer_t timer, struct itimerspec *time) {
    int ret = (int) syscall(SYS_TIMER_GETTIME, timer, time);
    __SYSCALL_TO_ERRNO(ret);
}
