#include <errno.h>
#include <signal.h>

int sigismember(const sigset_t *set, int signum) {
    if (signum < 1 || signum >= _NSIG) {
        errno = EINVAL;
        return -1;
    }
    return *set & (1 << (signum - 1)) ? 1 : 0;
}