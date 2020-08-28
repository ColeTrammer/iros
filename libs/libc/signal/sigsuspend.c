#include <errno.h>
#include <signal.h>
#include <sys/syscall.h>

int sigsuspend(const sigset_t *set) {
    int ret = (int) syscall(SYS_sigsuspend, set);
    __SYSCALL_TO_ERRNO(ret);
}
