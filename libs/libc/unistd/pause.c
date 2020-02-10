#include <signal.h>
#include <unistd.h>

int pause(void) {
    sigset_t set;
    sigprocmask(SIG_SETMASK, NULL, &set);
    return sigsuspend(&set);
}