#include <unistd.h>

int usleep(useconds_t usec) {
    struct timespec t = { .tv_sec = usec / 1000, .tv_nsec = usec * 1000000 };
    return nanosleep(&t, NULL);
}