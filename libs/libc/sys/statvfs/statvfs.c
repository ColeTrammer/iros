#include <errno.h>
#include <sys/statvfs.h>
#include <sys/syscall.h>

int statvfs(const char *__restrict path, struct statvfs *__restrict stat_buf) {
    int ret = (int) syscall(SYS_statvfs, path, stat_buf);
    __SYSCALL_TO_ERRNO(ret);
}
