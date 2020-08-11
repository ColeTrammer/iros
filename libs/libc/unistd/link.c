#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

int link(const char *newpath, const char *oldpath) {
    int ret = (int) syscall(SYS_LINK, newpath, oldpath);
    __SYSCALL_TO_ERRNO(ret);
}
