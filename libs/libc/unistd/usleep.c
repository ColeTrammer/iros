#include <time.h>
#include <unistd.h>

int usleep(useconds_t usec) {
    struct timespec t = { .tv_sec = usec / 1000000, .tv_nsec = (usec % 1000000) * 1000 };
    return nanosleep(&t, NULL);
}
