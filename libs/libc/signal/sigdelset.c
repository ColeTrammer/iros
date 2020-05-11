#include <errno.h>
#include <signal.h>
#include <stdint.h>

int sigdelset(sigset_t *set, int signum) {
    if (signum < 1 || signum >= _NSIG) {
        errno = EINVAL;
        return -1;
    }
    *set &= ~(UINT64_C(1) << (signum - UINT64_C(1)));
    return 0;
}
