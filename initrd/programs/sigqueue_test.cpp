#include <signal.h>
#include <stdio.h>
#include <unistd.h>

int main() {
    struct sigaction act;
    act.sa_flags = SA_SIGINFO | SA_RESTART;
    sigfillset(&act.sa_mask);
    act.sa_sigaction = [](int, siginfo_t *info, void *) {
        printf("[ %d ]: val %d\n", info->si_signo, info->si_value.sival_int);
    };

    int s1 = SIGRTMIN;
    int s2 = SIGRTMIN + 1;

    sigaction(s1, &act, nullptr);
    sigaction(s2, &act, nullptr);

    sigset_t set;
    sigprocmask(0, NULL, &set);
    sigaddset(&set, s1);
    sigaddset(&set, s2);
    sigprocmask(SIG_SETMASK, &set, NULL);

    pid_t pid = getpid();

    union sigval val;
    val.sival_int = 42;
    sigqueue(pid, s2, val);

    val.sival_int = 43;
    sigqueue(pid, s2, val);

    val.sival_int = 44;
    sigqueue(pid, s2, val);

    val.sival_int = 41;
    sigqueue(pid, s1, val);

    sigdelset(&set, s1);
    sigdelset(&set, s2);
    sigprocmask(SIG_SETMASK, &set, NULL);
}