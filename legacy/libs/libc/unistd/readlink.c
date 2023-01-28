#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

ssize_t readlink(const char *__restrict pathname, char *__restrict buf, size_t bufsiz) {
    ssize_t ret = (ssize_t) syscall(SYS_readlink, pathname, buf, bufsiz);
    __SYSCALL_TO_ERRNO(ret);
}
