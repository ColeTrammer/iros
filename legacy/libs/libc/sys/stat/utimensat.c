#include <errno.h>
#include <sys/stat.h>
#include <sys/syscall.h>

int utimensat(int fd, const char *path, const struct timespec times[2], int flags) {
    int ret = (int) syscall(SYS_utimensat, fd, path, times, flags);
    __SYSCALL_TO_ERRNO(ret);
}
