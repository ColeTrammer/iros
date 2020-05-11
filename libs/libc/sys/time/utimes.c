#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>

int utimes(const char *filename, const struct timeval times[2]) {
    struct timespec tms[2] = { { times[0].tv_sec, times[0].tv_usec * 1000 }, { times[1].tv_sec, times[1].tv_usec * 1000 } };
    return utimensat(AT_FDCWD, filename, tms, 0);
}
