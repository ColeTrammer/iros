#include <signal.h>

sighandler_t signal(int signum, sighandler_t handler) {
    struct sigaction act;
    struct sigaction old;

    act.sa_handler = handler;
    act.sa_flags = SA_RESTART; // glibc does this
    sigemptyset(&act.sa_mask);

    if (sigaction(signum, &act, &old)) {
        return SIG_ERR;
    }

    return old.sa_handler;
}
