#include <errno.h>
#include <sys/syscall.h>
#include <time.h>

extern "C" {
int timer_getoverrun(timer_t timer) {
    int ret = (int) syscall(SC_TIMER_GETOVERRUN, timer);
    __SYSCALL_TO_ERRNO(ret);
}
}