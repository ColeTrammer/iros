#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset) {
    ssize_t ret = (ssize_t) syscall(SC_PWRITE, fd, buf, count, offset);
    __SYSCALL_TO_ERRNO(ret);
}