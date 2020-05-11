#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

int fchdir(int fd) {
    int ret = (int) syscall(SC_FCHDIR, fd);
    __SYSCALL_TO_ERRNO(ret);
}
