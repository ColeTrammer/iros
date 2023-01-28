#include <sys/stat.h>

int mkfifo(const char *pathname, mode_t mode) {
    return mknod(pathname, (mode & 07777) | S_IFIFO, 0);
}