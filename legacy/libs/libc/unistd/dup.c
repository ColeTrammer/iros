#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

int dup(int oldfd) {
    int ret = (int) syscall(SYS_dup, oldfd);
    __SYSCALL_TO_ERRNO(ret);
}
