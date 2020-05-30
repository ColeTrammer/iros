#include <errno.h>
#include <stddef.h>
#include <sys/select.h>
#include <sys/syscall.h>

int pselect(int numfds, fd_set *__restrict readfds, fd_set *__restrict writefds, fd_set *__restrict exceptfds,
            const struct timespec *__restrict timeout, const sigset_t *__restrict sigmask) {
    int ret = (int) syscall(SC_PSELECT, numfds, readfds, writefds, exceptfds, timeout, sigmask);
    __SYSCALL_TO_ERRNO(ret);
}

int select(int numfds, fd_set *__restrict readfds, fd_set *__restrict writefds, fd_set *__restrict exceptfds,
           struct timeval *__restrict timeout) {
    if (timeout) {
        struct timespec timeout_as_timespec = { .tv_sec = timeout->tv_sec, .tv_nsec = timeout->tv_usec * 1000 };
        return pselect(numfds, readfds, writefds, exceptfds, &timeout_as_timespec, NULL);
    }
    return pselect(numfds, readfds, writefds, exceptfds, NULL, NULL);
}
