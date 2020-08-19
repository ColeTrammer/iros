#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>

#include <kernel/fs/vfs.h>
#include <kernel/hal/processor.h>
#include <kernel/net/socket.h>
#include <kernel/net/socket_syscalls.h>
#include <kernel/proc/process.h>
#include <kernel/proc/task.h>

static int socket_file_close(struct file *file);
static ssize_t net_read(struct file *file, off_t offset, void *buf, size_t len);
static ssize_t net_write(struct file *file, off_t offset, const void *buf, size_t len);

static struct file_operations socket_file_ops = { .close = socket_file_close, .read = net_read, .write = net_write };

static struct socket *socket_from_file(struct file *file) {
    return file->private_data;
}

static int socket_file_close(struct file *file) {
    struct socket *socket = socket_from_file(file);

#ifdef SOCKET_DEBUG
    debug_log("Destroying socket: [ %lu ]\n", socket->id);
#endif /* SOCKET_DEBUG */

    int ret = 0;
    if (socket->op->close) {
        socket->op->close(socket);
    }
    net_destroy_socket(socket);
    return ret;
}

static ssize_t net_read(struct file *file, off_t offset, void *buf, size_t len) {
    assert(offset == 0);
    return net_recvfrom(file, buf, len, 0, NULL, NULL);
}

static ssize_t net_write(struct file *file, off_t offset, const void *buf, size_t len) {
    assert(offset == 0);
    return net_sendto(file, buf, len, 0, NULL, 0);
}

struct socket *net_create_socket_fd(int domain, int type, int protocol, struct socket_ops *op, int *fd, void *private_data) {
    struct task *current = get_current_task();

    struct socket *socket = net_create_socket(domain, type, protocol, op, private_data);
    if (!socket) {
        *fd = -ENOMEM;
        return NULL;
    }

    for (int i = 0; i < FOPEN_MAX; i++) {
        if (current->process->files[i].file == NULL) {
            current->process->files[i].file = fs_create_file(NULL, FS_SOCKET, FS_FILE_CANT_SEEK, O_RDWR, &socket_file_ops, socket);
            current->process->files[i].fd_flags = (type & SOCK_CLOEXEC) ? FD_CLOEXEC : 0;

            *fd = i;
            return socket;
        }
    }

    net_destroy_socket(socket);
    *fd = -EMFILE;
    return NULL;
}

int net_accept(struct file *file, struct sockaddr *addr, socklen_t *addrlen, int flags) {
    struct socket *socket = socket_from_file(file);
    if (socket->state != LISTENING) {
        return -EINVAL;
    }

    if ((socket->type & SOCK_TYPE_MASK) != SOCK_STREAM) {
        return -EOPNOTSUPP;
    }

    if (!socket->op->accept) {
        return -EINVAL;
    }

    return socket->op->accept(socket, addr, addrlen, flags);
}

int net_bind(struct file *file, const struct sockaddr *addr, socklen_t addrlen) {
    struct socket *socket = socket_from_file(file);
    if (addr->sa_family != socket->domain || addrlen > sizeof(struct sockaddr_storage)) {
        return -EINVAL;
    }

    if (!socket->op->bind) {
        return -EINVAL;
    }

    return socket->op->bind(socket, addr, addrlen);
}

int net_connect(struct file *file, const struct sockaddr *addr, socklen_t addrlen) {
    if (addrlen > sizeof(struct sockaddr_storage)) {
        return -EINVAL;
    }

    struct socket *socket = socket_from_file(file);

    if (addr->sa_family != socket->domain) {
        return -EAFNOSUPPORT;
    }

    if (!socket->op->connect) {
        return -EINVAL;
    }

    return socket->op->connect(socket, addr, addrlen);
}

int net_getpeername(struct file *file, struct sockaddr *addr, socklen_t *addrlen) {
    struct socket *socket = socket_from_file(file);
    if (!socket->op->getpeername) {
        return -EINVAL;
    }

    return socket->op->getpeername(socket, addr, addrlen);
}

int net_getsockname(struct file *file, struct sockaddr *addr, socklen_t *addrlen) {
    struct socket *socket = socket_from_file(file);
    if (!socket->op->getsockname) {
        return -EINVAL;
    }

    return socket->op->getsockname(socket, addr, addrlen);
}

int net_listen(struct file *file, int backlog) {
    struct socket *socket = socket_from_file(file);
    if (backlog <= 0 || backlog > SOMAXCONN || socket->state != BOUND) {
        return -EINVAL;
    }

    if (!socket->op->listen) {
        return -EOPNOTSUPP;
    }

    return socket->op->listen(socket, backlog);
}

int net_getsockopt(struct file *file, int level, int optname, void *optval, socklen_t *optlen) {
    struct socket *socket = socket_from_file(file);
    if (!socket->op->getsockopt) {
        return -EINVAL;
    }

    return socket->op->getsockopt(socket, level, optname, optval, optlen);
}

int net_setsockopt(struct file *file, int level, int optname, const void *optval, socklen_t optlen) {
    struct socket *socket = socket_from_file(file);
    if (!socket->op->setsockopt) {
        return -EINVAL;
    }

    return socket->op->setsockopt(socket, level, optname, optval, optlen);
}

int net_socket(int domain, int type, int protocol) {
    if (type == SOCK_RAW && get_current_process()->euid != 0) {
        return -EACCES;
    }

    bool saw_af_family = false;
    net_for_each_protocol(iter) {
        if (iter->domain == domain) {
            if (iter->type == (type & SOCK_TYPE_MASK) && (iter->protocol == protocol || (protocol == 0 && iter->is_default_protocol))) {
                return iter->create_socket(domain, type, iter->protocol);
            }

            saw_af_family = true;
        }
    }

    return saw_af_family ? -EPROTONOSUPPORT : -EAFNOSUPPORT;
}

int net_socketpair(int domain, int type, int protocol, int *fds) {
    bool saw_af_family = false;
    net_for_each_protocol(iter) {
        if (iter->domain == domain) {
            if (iter->type == (type & SOCK_TYPE_MASK) && (iter->protocol == protocol || (protocol == 0 && iter->is_default_protocol))) {
                if (!iter->create_socket_pair) {
                    return -EOPNOTSUPP;
                }
                return iter->create_socket_pair(domain, type, iter->protocol, fds);
            }

            saw_af_family = true;
        }
    }

    return saw_af_family ? -EPROTONOSUPPORT : -EAFNOSUPPORT;
}

ssize_t net_sendto(struct file *file, const void *buf, size_t len, int flags, const struct sockaddr *dest, socklen_t addrlen) {
    if (!dest && !!addrlen) {
        return -EINVAL;
    }

    if (!!dest && addrlen > sizeof(struct sockaddr_storage)) {
        return -EINVAL;
    }

    struct socket *socket = socket_from_file(file);
    if (!!dest && dest->sa_family != socket->domain) {
        return -EINVAL;
    }

    if (!socket->op->sendto) {
        return -EINVAL;
    }

    return socket->op->sendto(socket, buf, len, flags, dest, addrlen);
}

ssize_t net_recvfrom(struct file *file, void *buf, size_t len, int flags, struct sockaddr *source, socklen_t *addrlen) {
    if ((!!source && !addrlen) || (!source && !!addrlen)) {
        return -EINVAL;
    }

    struct socket *socket = socket_from_file(file);
    if (!socket->op->recvfrom) {
        return -EINVAL;
    }

    return socket->op->recvfrom(socket, buf, len, flags, source, addrlen);
}
