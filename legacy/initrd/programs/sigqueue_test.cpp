#include <signal.h>
#include <stdio.h>
#include <unistd.h>

int main() {
    struct sigaction act;
    act.sa_flags = SA_SIGINFO | SA_RESTART;
    sigfillset(&act.sa_mask);
    act.sa_sigaction = [](int, siginfo_t *info, void *) {
        printf("[ %d ]: val %d\n", info->si_signo, info->si_value.sival_int);
        psiginfo(info, "handler");
    };

    int s1 = SIGRTMIN;
    int s2 = SIGRTMIN + 1;

    sigaction(s1, &act, nullptr);
    sigaction(s2, &act, nullptr);
    if (sigaction(SIGUSR1, &act, nullptr)) {
        perror("sigaction");
        return 1;
    }

    act.sa_handler = [](int) {
        printf("sigusr2\n");
    };

    act.sa_flags &= ~SA_SIGINFO;
    sigaction(SIGUSR2, &act, nullptr);

    sigset_t set;
    sigprocmask(0, NULL, &set);
    sigaddset(&set, s1);
    sigaddset(&set, s2);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGUSR2);
    sigprocmask(SIG_SETMASK, &set, NULL);

    pid_t pid = getpid();

    kill(pid, s2);

    union sigval val;
    val.sival_int = 42;
    sigqueue(pid, s2, val);

    val.sival_int = 43;
    sigqueue(pid, s2, val);

    val.sival_int = 44;
    sigqueue(pid, s2, val);

    val.sival_int = 41;
    sigqueue(pid, s1, val);

    kill(pid, s2);
    kill(pid, s2);

    val.sival_int = 40;
    kill(pid, SIGUSR1);
    sigqueue(pid, SIGUSR1, val);
    sigqueue(pid, SIGUSR1, val);
    sigqueue(pid, SIGUSR1, val);
    kill(pid, SIGUSR1);
    kill(pid, SIGUSR1);

    kill(pid, SIGUSR2);
    kill(pid, SIGUSR2);
    kill(pid, SIGUSR2);

    sigdelset(&set, s1);
    sigdelset(&set, s2);
    sigdelset(&set, SIGUSR1);
    sigdelset(&set, SIGUSR2);
    sigprocmask(SIG_SETMASK, &set, NULL);
}
