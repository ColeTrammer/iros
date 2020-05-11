#include <errno.h>
#include <signal.h>

int siginterrupt(int sig, int flag) {
    struct sigaction act;

    if (sig <= 0 || sig >= _NSIG) {
        errno = -EINVAL;
        return -1;
    }

    sigaction(sig, NULL, &act);
    if (flag == 0) {
        act.sa_flags |= SA_RESTART;
    } else {
        act.sa_flags &= ~SA_RESTART;
    }

    return sigaction(sig, &act, NULL);
}
