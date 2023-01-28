#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

int pipe(int pipefd[2]) {
    int ret = (int) syscall(SYS_pipe, pipefd);
    __SYSCALL_TO_ERRNO(ret);
}
