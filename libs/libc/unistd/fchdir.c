#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

int fchdir(int fd) {
    int ret = (int) syscall(SYS_fchdir, fd);
    __SYSCALL_TO_ERRNO(ret);
}
