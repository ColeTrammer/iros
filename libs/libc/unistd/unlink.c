#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

int unlink(const char *pathname) {
    int ret = (int) syscall(SYS_UNLINK, pathname);
    __SYSCALL_TO_ERRNO(ret);
}
