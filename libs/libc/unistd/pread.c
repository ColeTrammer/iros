#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

ssize_t pread(int fd, void *buf, size_t count, off_t offset) {
    ssize_t ret = (ssize_t) syscall(SYS_PREAD, fd, buf, count, offset);
    __SYSCALL_TO_ERRNO(ret);
}
