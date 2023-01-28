#include <errno.h>
#include <sys/syscall.h>
#include <time.h>

int timer_gettime(timer_t timer, struct itimerspec *time) {
    int ret = (int) syscall(SYS_timer_gettime, timer, time);
    __SYSCALL_TO_ERRNO(ret);
}
