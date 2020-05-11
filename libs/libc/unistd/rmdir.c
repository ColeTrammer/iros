#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

int rmdir(const char *pathname) {
    int ret = (int) syscall(SC_RMDIR, pathname);
    __SYSCALL_TO_ERRNO(ret);
}
