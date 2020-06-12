#include <fcntl.h>
#include <sys/stat.h>

int fchmod(int fd, mode_t mode) {
    return fchmodat(fd, "", mode, AT_EMPTY_PATH);
}
