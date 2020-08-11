#include <errno.h>
#include <sys/stat.h>
#include <sys/syscall.h>

int mknod(const char *path, mode_t mode, dev_t dev) {
    int ret = (int) syscall(SYS_MKNOD, path, mode, dev);
    __SYSCALL_TO_ERRNO(ret);
}
