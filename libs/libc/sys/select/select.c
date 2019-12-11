#include <stddef.h>
#include <sys/select.h>
#include <sys/syscall.h>

int pselect(int numfds, fd_set *__restrict readfds, fd_set *__restrict writefds, fd_set *__restrict exceptfds,
            const struct timespec *__restrict timeout, const sigset_t *__restrict sigmask) {
    int ret = (int) syscall(SC_PSELECT, numfds, readfds, writefds, exceptfds, timeout, sigmask);
}

int select(int numfds, fd_set *__restrict readfds, fd_set *__restrict writefds, fd_set *__restrict exceptfds,
           const struct timespec *__restrict timeout) {
    return pselect(numfds, readfds, writefds, exceptfds, timeout, NULL);
}