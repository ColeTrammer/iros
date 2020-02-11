#include <errno.h>
#include <signal.h>
#include <sys/syscall.h>

int sigaltstack(const stack_t *ss, stack_t *old_ss) {
    int ret = (int) syscall(SC_SIGALTSTACK, ss, old_ss);
    __SYSCALL_TO_ERRNO(ret);
}