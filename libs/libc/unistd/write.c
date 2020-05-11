#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

ssize_t write(int fd, const void *buf, size_t count) {
    ssize_t ret = syscall(SC_WRITE, fd, buf, count);
    __SYSCALL_TO_ERRNO(ret);
}
