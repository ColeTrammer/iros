#include <stddef.h>
#include <sys/socket.h>

int accept(int fd, struct sockaddr *__restrict addr, socklen_t *__restrict addrlen) {
    return accept4(fd, addr, addrlen, 0);
}

ssize_t send(int fd, const void *buf, size_t len, int flags) {
    return sendto(fd, buf, len, flags, NULL, 0);
}

ssize_t recv(int fd, void *buf, size_t len, int flags) {
    return recvfrom(fd, buf, len, flags, NULL, NULL);
}