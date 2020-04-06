#include <errno.h>
#include <sys/stat.h>
#include <sys/syscall.h>

int fstatat(int fd, const char *__restrict path, struct stat *__restrict stat_struct, int flag) {
    (void) fd;
    (void) path;
    (void) stat_struct;
    (void) flag;
    errno = ENOTSUP;
    return -1;
}