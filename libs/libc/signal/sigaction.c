#include <errno.h>
#include <signal.h>
#include <sys/syscall.h>

int sigaction(int signum, const struct sigaction *act, struct sigaction *old_act) {
    ((struct sigaction *) act)->sa_restorer = &__sigreturn;
    int ret = (int) syscall(SC_SIGACTION, signum, act, old_act);
    __SYSCALL_TO_ERRNO(ret);
}
