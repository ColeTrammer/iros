#include <sys/time.h>
#include <unistd.h>

useconds_t ualarm(useconds_t useconds, useconds_t interval) {
    struct itimerval nval = (struct itimerval) {
        .it_value = {
            .tv_sec = useconds / 1000000,
            .tv_usec = useconds % 1000000,
        },
        .it_interval = {
            .tv_sec = interval / 1000000,
            .tv_usec = interval % 1000000,
        },
    };
    struct itimerval oval;
    setitimer(ITIMER_REAL, &nval, &oval);
    return oval.it_value.tv_sec * 1000000 + oval.it_value.tv_usec;
}
