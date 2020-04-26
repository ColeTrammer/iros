#include <time.h>
#include <unistd.h>

int nanosleep(const struct timespec *req, struct timespec *rem) {
    return clock_nanosleep(CLOCK_REALTIME, 0, req, rem);
}