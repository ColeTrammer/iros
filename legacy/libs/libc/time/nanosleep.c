#include <time.h>

int nanosleep(const struct timespec *amount, struct timespec *remaining) {
    return clock_nanosleep(CLOCK_REALTIME, 0, amount, remaining);
}
