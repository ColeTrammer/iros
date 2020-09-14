#include <sys/time.h>
#include <unistd.h>

unsigned int alarm(unsigned int seconds) {
    struct itimerval nval = (struct itimerval) {
        .it_value = {
            .tv_sec = seconds,
        },
    };
    struct itimerval oval;
    setitimer(ITIMER_REAL, &nval, &oval);
    return oval.it_value.tv_sec;
}
