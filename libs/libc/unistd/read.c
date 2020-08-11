#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

ssize_t read(int fd, void *buf, size_t count) {
    ssize_t ret = syscall(SYS_READ, fd, buf, count);
    __SYSCALL_TO_ERRNO(ret);
}
