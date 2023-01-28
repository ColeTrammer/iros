#include <errno.h>
#include <signal.h>
#include <sys/syscall.h>

int sigprocmask(int how, const sigset_t *set, sigset_t *old) {
    int ret = (int) syscall(SYS_sigprocmask, how, set, old);
    __SYSCALL_TO_ERRNO(ret);
}
