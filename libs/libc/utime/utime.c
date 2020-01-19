#include <stdio.h>
#include <sys/time.h>
#include <utime.h>

int utime(const char *filename, const struct utimbuf *times) {
    struct timeval time[2] = { (struct timeval) { .tv_sec = times->actime, .tv_usec = 0 },
                               (struct timeval) { .tv_sec = times->modtime, .tv_usec = 0 } };
    return utimes(filename, time);
}