#ifndef _KERNEL_NET_SOCKET_SYSCALLS_H
#define _KERNEL_NET_SOCKET_SYSCALLS_H 1

#include <sys/socket.h>

struct file;
struct sockaddr;
struct socket;
struct socket_ops;

struct socket *net_create_socket_fd(int domain, int type, int protocol, struct socket_ops *op, int *fd, void *private_data);

int net_accept(struct file *file, struct sockaddr *addr, socklen_t *addrlen, int flags);
int net_bind(struct file *file, const struct sockaddr *addr, socklen_t addrlen);
int net_connect(struct file *file, const struct sockaddr *addr, socklen_t addrlen);
int net_listen(struct file *file, int backlog);
int net_setsockopt(struct file *file, int level, int optname, const void *optval, socklen_t optlen);
int net_socket(int domain, int type, int protocol);
int net_getpeername(struct file *file, struct sockaddr *addr, socklen_t *addrlen);
ssize_t net_sendto(struct file *file, const void *buf, size_t len, int flags, const struct sockaddr *dest, socklen_t addrlen);
ssize_t net_recvfrom(struct file *file, void *buf, size_t len, int flags, struct sockaddr *source, socklen_t *addrlen);

#endif /* _KERNEL_NET_SOCKET_SYSCALLS_H */
