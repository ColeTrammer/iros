#include <fcntl.h>
#include <sys/stat.h>

int futimens(int fd, const struct timespec times[2]) {
    return utimensat(fd, "", times, AT_EMPTY_PATH);
}
