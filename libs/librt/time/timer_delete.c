#include <errno.h>
#include <sys/syscall.h>
#include <time.h>

int timer_delete(timer_t timer) {
    int ret = (int) syscall(SYS_TIMER_DELETE, timer);
    __SYSCALL_TO_ERRNO(ret);
}
