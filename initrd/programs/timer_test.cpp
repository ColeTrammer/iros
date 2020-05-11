#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static int iii;
static timer_t timer;

int main() {
    struct sigaction act;
    act.sa_flags = SA_SIGINFO;
    sigemptyset(&act.sa_mask);
    act.sa_sigaction = [](int, siginfo_t*, void*) {
        printf("%d\n", iii++);
    };

    if (sigaction(SIGRTMIN, &act, nullptr)) {
        perror("sigaction");
        return 1;
    }

    sigevent ev;
    ev.sigev_signo = SIGRTMIN;
    ev.sigev_notify = SIGEV_SIGNAL;

    if (timer_create(CLOCK_MONOTONIC, &ev, &timer)) {
        perror("timer_create");
        return 1;
    }

    struct itimerspec spec;
    spec.it_value = { .tv_sec = 0, .tv_nsec = 500000000 };
    spec.it_interval = spec.it_value;
    if (timer_settime(timer, 0, &spec, nullptr)) {
        perror("time_settime");
        return 1;
    }

    for (;;) {
        sleep(1000);
    }
}
