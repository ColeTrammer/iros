#include <poll.h>
#include <stddef.h>

int poll(struct pollfd *fds, nfds_t nfds, int timeout) {
    struct timespec ts = { .tv_sec = timeout / 1000, .tv_nsec = (timeout % 1000) * 1000000 };
    return ppoll(fds, nfds, timeout < 0 ? NULL : &ts, NULL);
}
