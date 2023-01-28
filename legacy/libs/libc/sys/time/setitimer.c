#include <errno.h>
#include <sys/syscall.h>
#include <sys/time.h>

int setitimer(int which, const struct itimerval *nvalp, struct itimerval *ovalp) {
    int ret = (int) syscall(SYS_setitimer, which, nvalp, ovalp);
    __SYSCALL_TO_ERRNO(ret);
}
