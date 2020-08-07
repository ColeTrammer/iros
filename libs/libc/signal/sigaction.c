#include <errno.h>
#include <signal.h>
#include <sys/syscall.h>

int sigaction(int signum, const struct sigaction *_act, struct sigaction *old_act) {
    struct sigaction act = { 0 };
    if (_act) {
        act = *_act;
        act.sa_restorer = &__sigreturn;

        // Explitly settings SIG_DFL for SIGCHLD is the same as setting the SA_NOCLDWAIT flag.
        if (signum == SIGCHLD && act.sa_handler == SIG_DFL) {
            act.sa_flags |= SA_NOCLDWAIT;
        }
    }
    int ret = (int) syscall(SC_SIGACTION, signum, _act ? &act : NULL, old_act);
    __SYSCALL_TO_ERRNO(ret);
}
