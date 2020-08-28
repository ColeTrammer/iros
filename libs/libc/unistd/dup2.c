#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

int dup2(int oldfd, int newfd) {
    int ret = (int) syscall(SYS_dup2, oldfd, newfd);
    __SYSCALL_TO_ERRNO(ret);
}
