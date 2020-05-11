#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

int dup2(int oldfd, int newfd) {
    int ret = (int) syscall(SC_DUP2, oldfd, newfd);
    __SYSCALL_TO_ERRNO(ret);
}
