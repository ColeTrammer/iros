#include <fcntl.h>
#include <sys/stat.h>
#include <utime.h>

int utime(const char *filename, const struct utimbuf *times) {
    struct timespec ts[2] = { (struct timespec) { .tv_sec = times->actime, .tv_nsec = 0 },
                              (struct timespec) { .tv_sec = times->modtime, .tv_nsec = 0 } };
    return utimensat(AT_FDCWD, filename, ts, 0);
}