#include <errno.h>
#include <sys/syscall.h>
#include <sys/wait.h>

pid_t wait(int *wstatus) {
    return waitpid(-1, wstatus, 0);
}

pid_t waitpid(pid_t pid, int *wstatus, int options) {
    pid_t ret = (pid_t) syscall(SYS_WAITPID, pid, wstatus, options);
    __SYSCALL_TO_ERRNO(ret);
}
