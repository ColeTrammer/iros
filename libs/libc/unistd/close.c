#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

int close(int fd) {
    int ret = (int) syscall(SYS_close, fd);
    __SYSCALL_TO_ERRNO(ret);
}
