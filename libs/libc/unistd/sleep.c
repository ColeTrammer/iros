#include <errno.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

unsigned int sleep(unsigned int seconds) {
    struct timespec sec = { .tv_sec = seconds, .tv_nsec = 0 };
    struct timespec rem;
    int ret = (int) syscall(SC_CLOCK_NANOSLEEP, CLOCK_REALTIME, 0, &sec, &rem);
    if (ret == -EINTR) {
        return rem.tv_sec + rem.tv_nsec ? 1 : 0;
    }

    return 0;
}