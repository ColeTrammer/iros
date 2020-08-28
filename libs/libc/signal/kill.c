#include <errno.h>
#include <signal.h>
#include <sys/syscall.h>

int kill(pid_t pid, int sig) {
    int ret = (int) syscall(SYS_kill, pid, sig);
    __SYSCALL_TO_ERRNO(ret);
}
