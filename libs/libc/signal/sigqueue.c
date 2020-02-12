#include <errno.h>
#include <signal.h>
#include <sys/syscall.h>

int sigqueue(pid_t pid, int sig, const union sigval value) {
    int ret = (int) syscall(SC_SIGQUEUE, pid, sig, value.sival_ptr);
    __SYSCALL_TO_ERRNO(ret);
}