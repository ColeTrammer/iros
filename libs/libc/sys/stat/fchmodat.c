#include <errno.h>
#include <sys/stat.h>
#include <sys/syscall.h>

int fchmodat(int dirfd, const char *pathname, mode_t mode, int flags) {
    int ret = syscall(SYS_fchmodat, dirfd, pathname, mode, flags);
    __SYSCALL_TO_ERRNO(ret);
}
