#include <errno.h>
#include <sys/syscall.h>
#include <time.h>

int timer_getoverrun(timer_t timer) {
    int ret = (int) syscall(SYS_timer_getoverrun, timer);
    __SYSCALL_TO_ERRNO(ret);
}
