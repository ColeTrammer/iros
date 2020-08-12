#include <errno.h>
#include <sys/resource.h>
#include <sys/syscall.h>

int setrlimit(int what, const struct rlimit *res) {
    int ret = (int) syscall(SYS_SETRLIMIT, what, res);
    __SYSCALL_TO_ERRNO(ret);
}
