#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <kernel/fs/vfs.h>
#include <kernel/proc/process.h>
#include <kernel/net/socket.h>

int net_unix_bind(struct socket *socket, const struct sockaddr_un *addr, socklen_t addrlen) {
    assert(socket->domain == AF_UNIX);
    assert(addr);

    if (addr->sun_family != AF_UNIX) {
        return -EINVAL;
    }

    if (addrlen <= offsetof(struct sockaddr_un, sun_path) || addr->sun_path[0] != '/') {
        return -EINVAL;
    }

    char path[UNIX_PATH_MAX];
    strncpy(path, addr->sun_path, UNIX_PATH_MAX);

    if (iname(path)) {
        return -EADDRINUSE;
    }

    int ret = fs_create(path, S_IFSOCK | 0666);
    if (ret == -1) {
        return ret;
    }

    struct tnode *tnode = iname(path);
    ret = fs_bind_socket_to_inode(tnode->inode, socket->id);
    if (ret == -1) {
        return ret;
    }

    socket->state = BOUND;
    return 0;
}

int net_unix_socket(int domain, int type, int protocol) {
    if (type != SOCK_STREAM || protocol != 0) {
        return -EPROTONOSUPPORT;
    }

    int fd;
    struct socket *socket = net_create_socket(domain, type, protocol, &fd);
    (void) socket;

    return fd;
}