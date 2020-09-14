#include <errno.h>
#include <sys/syscall.h>
#include <sys/time.h>

int getitimer(int which, struct itimerval *value) {
    int ret = (int) syscall(SYS_getitimer, which, value);
    __SYSCALL_TO_ERRNO(ret);
}
