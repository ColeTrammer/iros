#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

off_t lseek(int fd, off_t offset, int whence) {
    off_t ret = syscall(SYS_LSEEK, fd, offset, whence);
    __SYSCALL_TO_ERRNO(ret);
}
