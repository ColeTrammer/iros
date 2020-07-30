#include <errno.h>
#include <sys/syscall.h>
#include <time.h>

int timer_settime(timer_t timer, int flags, const struct itimerspec *__restrict new_time, struct itimerspec *__restrict old) {
    int ret = (int) syscall(SC_TIMER_SETTIME, timer, flags, new_time, old);
    __SYSCALL_TO_ERRNO(ret);
}
