#include <errno.h>
#include <sys/socket.h>
#include <sys/syscall.h>

ssize_t sendmsg(int fd, const struct msghdr *message, int flags) {
    (void) fd;
    (void) message;
    (void) flags;
    return -ENOSYS;
}
