#include <errno.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sys/syscall.h>

int accept4(int fd, struct sockaddr *__restrict addr, socklen_t *__restrict addrlen, int flags) {
    int ret = (int) syscall(SC_ACCEPT4, fd, addr, addrlen, flags);
    __SYSCALL_TO_ERRNO(ret);
}

int bind(int fd, const struct sockaddr *addr, socklen_t addrlen) {
    int ret = (int) syscall(SC_BIND, fd, addr, addrlen);
    __SYSCALL_TO_ERRNO(ret);
}

int connect(int fd, const struct sockaddr *addr, socklen_t addrlen) {
    int ret = (int) syscall(SC_CONNECT, fd, addr, addrlen);
    __SYSCALL_TO_ERRNO(ret);
}

int listen(int fd, int backlog) {
    int ret = (int) syscall(SC_LISTEN, fd, backlog);
    __SYSCALL_TO_ERRNO(ret);
}

int socket(int domain, int type, int protocol) {
    int ret = (int) syscall(SC_SOCKET, domain, type, protocol);
    __SYSCALL_TO_ERRNO(ret);
}

int shutdown(int fd, int how) {
    int ret = (int) syscall(SC_SHUTDOWN, fd, how);
    __SYSCALL_TO_ERRNO(ret);
}

int getsockopt(int fd, int level, int optname, void *__restrict optval, socklen_t *__restrict optlen) {
    int ret = (int) syscall(SC_GETSOCKOPT, fd, level, optname, optval, optlen);
    __SYSCALL_TO_ERRNO(ret);
}

int setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen) {
    int ret = (int) syscall(SC_SETSOCKOPT, fd, level, optname, optval, optlen);
    __SYSCALL_TO_ERRNO(ret);
}

int getpeername(int fd, struct sockaddr *__restrict addr, socklen_t *__restrict addrlen) {
    int ret = (int) syscall(SC_GETPEERNAME, fd, addr, addrlen);
    __SYSCALL_TO_ERRNO(ret);
}

int getsockname(int fd, struct sockaddr *__restrict addr, socklen_t *__restrict addrlen) {
    int ret = (int) syscall(SC_GETSOCKNAME, fd, addr, addrlen);
    __SYSCALL_TO_ERRNO(ret);
}

ssize_t sendto(int fd, const void *buf, size_t len, int flags, const struct sockaddr *dest, socklen_t addrlen) {
    ssize_t ret = syscall(SC_SENDTO, fd, buf, len, flags, dest, addrlen);
    __SYSCALL_TO_ERRNO(ret);
}

ssize_t recvfrom(int fd, void *buf, size_t len, int flags, struct sockaddr *__restrict source, socklen_t *__restrict addrlen) {
    ssize_t ret = syscall(SC_RECVFROM, fd, buf, len, flags, source, addrlen);
    __SYSCALL_TO_ERRNO(ret);
}

int accept(int fd, struct sockaddr *__restrict addr, socklen_t *__restrict addrlen) {
    return accept4(fd, addr, addrlen, 0);
}

ssize_t send(int fd, const void *buf, size_t len, int flags) {
    return sendto(fd, buf, len, flags, NULL, 0);
}

ssize_t recv(int fd, void *buf, size_t len, int flags) {
    return recvfrom(fd, buf, len, flags, NULL, NULL);
}