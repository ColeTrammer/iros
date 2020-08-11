#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

int fchownat(int dirfd, const char *pathname, uid_t owner, gid_t group, int flags) {
    int ret = (int) syscall(SYS_FCHOWNAT, dirfd, pathname, owner, group, flags);
    __SYSCALL_TO_ERRNO(ret);
}
