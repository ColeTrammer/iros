#include <errno.h>
#include <signal.h>
#include <sys/syscall.h>

int sigpending(sigset_t *set) {
    int ret = (int) syscall(SC_SIGPENDING, set);
    __SYSCALL_TO_ERRNO(ret);
}