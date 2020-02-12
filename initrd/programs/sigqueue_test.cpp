#include <signal.h>
#include <stdio.h>
#include <unistd.h>

int main() {
    struct sigaction act;
    act.sa_flags = SA_SIGINFO | SA_RESTART;
    sigfillset(&act.sa_mask);
    act.sa_sigaction = [](int, siginfo_t *info, void *) {
        printf("val %d\n", info->si_value.sival_int);
    };
    ::sigaction(SIGRTMIN, &act, nullptr);

    sigset_t set;
    sigprocmask(0, NULL, &set);
    sigaddset(&set, SIGRTMIN);
    sigprocmask(SIG_SETMASK, &set, NULL);

    union sigval val;
    val.sival_int = 42;
    sigqueue(0, SIGRTMIN, val);

    val.sival_int = 43;
    sigqueue(0, SIGRTMIN, val);

    val.sival_int = 44;
    sigqueue(0, SIGRTMIN, val);

    sigdelset(&set, SIGRTMIN);
    sigprocmask(SIG_SETMASK, &set, NULL);
}