#include <signal.h>
#include <stdio.h>
#include <string.h>

void psiginfo(const siginfo_t *info, const char *s) {
    switch (info->si_code) {
        case SI_USER:
        case SI_QUEUE:
            fprintf(stderr, "%s: %s (Signal sent by %s() %d %hu)\n", s, strsignal(info->si_signo),
                    info->si_code == SI_USER ? "kill" : "sigqueue", info->si_pid, info->si_uid);
            break;
        default:
            fprintf(stderr, "%s: %s (unknown siginfo code)\n", s, strsignal(info->si_signo));
            break;
    }
}
