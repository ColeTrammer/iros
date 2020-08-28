#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

int chdir(const char *path) {
    int ret = (int) syscall(SYS_chdir, path);
    __SYSCALL_TO_ERRNO(ret);
}
