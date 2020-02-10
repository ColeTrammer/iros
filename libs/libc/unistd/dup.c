#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

int dup(int oldfd) {
    int ret = (int) syscall(SC_DUP, oldfd);
    __SYSCALL_TO_ERRNO(ret);
}