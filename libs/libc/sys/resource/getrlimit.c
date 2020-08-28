#include <errno.h>
#include <sys/resource.h>
#include <sys/syscall.h>

int getrlimit(int what, struct rlimit *res) {
    int ret = (int) syscall(SYS_getrlimit, what, res);
    __SYSCALL_TO_ERRNO(ret);
}
