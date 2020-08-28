#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

int faccessat(int dirfd, const char *path, int mode, int flags) {
    int ret = (int) syscall(SYS_faccessat, dirfd, path, mode, flags);
    __SYSCALL_TO_ERRNO(ret);
}
