#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

int access(const char *path, int mode) {
    int ret = (int) syscall(SC_ACCESS, path, mode);
    __SYSCALL_TO_ERRNO(ret);
}