#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

int unlink(const char *pathname) {
    int ret = (int) syscall(SC_UNLINK, pathname);
    __SYSCALL_TO_ERRNO(ret);
}
