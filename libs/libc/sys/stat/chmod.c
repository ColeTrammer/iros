#include <fcntl.h>
#include <sys/stat.h>

int chmod(const char *pathname, mode_t mode) {
    return fchmodat(AT_FDCWD, pathname, mode, 0);
}
