#include <errno.h>
#include <poll.h>
#include <sys/syscall.h>

int ppoll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout, const sigset_t *sigset) {
    int ret = (int) syscall(SYS_PPOLL, fds, nfds, timeout, sigset);
    __SYSCALL_TO_ERRNO(ret);
}
