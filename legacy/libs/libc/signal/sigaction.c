#include <errno.h>
#include <signal.h>
#include <sys/syscall.h>

int sigaction(int signum, const struct sigaction *_act, struct sigaction *old_act) {
    struct sigaction act = { 0 };
    if (_act) {
        act = *_act;
        act.sa_restorer = &__sigreturn;
    }
    int ret = (int) syscall(SYS_sigaction, signum, _act ? &act : NULL, old_act);
    __SYSCALL_TO_ERRNO(ret);
}
