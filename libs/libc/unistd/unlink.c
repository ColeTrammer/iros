#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

int unlink(const char *pathname) {
    int ret = (int) syscall(SYS_unlink, pathname);
    __SYSCALL_TO_ERRNO(ret);
}
