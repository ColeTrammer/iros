#include <errno.h>
#include <sys/statvfs.h>
#include <sys/syscall.h>

int fstatvfs(int fd, struct statvfs *stat_buf) {
    int ret = (int) syscall(SYS_FSTATVFS, fd, stat_buf);
    __SYSCALL_TO_ERRNO(ret);
}
