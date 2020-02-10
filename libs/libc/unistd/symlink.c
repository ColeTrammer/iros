#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

int symlink(const char *target, const char *linkpath) {
    int ret = (int) syscall(SC_SYMLINK, target, linkpath);
    __SYSCALL_TO_ERRNO(ret);
}