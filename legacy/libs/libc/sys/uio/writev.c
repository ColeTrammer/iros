#include <errno.h>
#include <sys/syscall.h>
#include <sys/uio.h>

ssize_t writev(int fd, const struct iovec *vec, int num) {
    ssize_t ret = (ssize_t) syscall(SYS_writev, fd, vec, num);
    __SYSCALL_TO_ERRNO(ret);
}
