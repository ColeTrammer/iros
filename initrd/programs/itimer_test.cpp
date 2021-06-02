#include <eventloop/event_loop.h>
#include <signal.h>
#include <stdio.h>
#include <sys/time.h>

int main() {
    using namespace App;

    static volatile sig_atomic_t x = 0;

    EventLoop loop;
    EventLoop::register_signal_handler(SIGALRM, [&] {
        printf("SIGALRM\n");
        x = x + 1;
        if (x == 5) {
            loop.set_should_exit(true);
        }
    });

    itimerval val;
    val.it_interval.tv_sec = 1;
    val.it_interval.tv_usec = 0;
    val.it_value = val.it_interval;
    if (setitimer(ITIMER_REAL, &val, nullptr) < 0) {
        perror("itimer_test: setitimer");
        return 1;
    }
    loop.enter();

    x = 0;
    signal(SIGPROF, [](auto) {
        printf("SIGPROF\n");
        x = x + 1;
    });
    if (setitimer(ITIMER_PROF, &val, nullptr) < 0) {
        perror("itimer_test: setitimer");
        return 1;
    }
    itimerval t;
    while (x != 5) {
        getitimer(ITIMER_PROF, &t);
        asm volatile("" ::: "memory");
    }
    t.it_value.tv_sec = 0;
    t.it_value.tv_usec = 0;
    if (setitimer(ITIMER_PROF, &t, nullptr) < 0) {
        perror("itimer_test: setitimer");
        return 1;
    }

    x = 0;
    signal(SIGVTALRM, [](auto) {
        printf("SIGVTALRM\n");
        x = x + 1;
    });
    sigset_t set;
    sigprocmask(0, 0, &set);
    if (setitimer(ITIMER_VIRTUAL, &val, nullptr) < 0) {
        perror("itimer_test: setitimer");
        return 1;
    }
    while (x != 5) {
        asm volatile("" ::: "memory");
    }

    return 0;
}
