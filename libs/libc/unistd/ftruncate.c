#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

int ftruncate(int fd, off_t length) {
    int ret = (int) syscall(SC_FTRUNCATE, fd, length);
    __SYSCALL_TO_ERRNO(ret);
}
