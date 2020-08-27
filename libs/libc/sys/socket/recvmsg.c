#include <errno.h>
#include <sys/socket.h>
#include <sys/syscall.h>

ssize_t recvmsg(int fd, struct msghdr *message, int flags) {
    (void) fd;
    (void) message;
    (void) flags;
    return -ENOSYS;
}
