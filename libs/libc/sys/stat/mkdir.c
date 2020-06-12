#include <errno.h>
#include <sys/stat.h>
#include <sys/syscall.h>

int mkdir(const char *path, mode_t mode) {
    int ret = (int) syscall(SC_MKDIR, path, mode);
    __SYSCALL_TO_ERRNO(ret);
}
