#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <kernel/fs/vfs.h>
#include <kernel/net/socket.h>
#include <kernel/net/unix_socket.h>
#include <kernel/proc/process.h>
#include <kernel/sched/process_sched.h>

int net_unix_accept(struct socket *socket, struct sockaddr_un *addr, socklen_t *addrlen) {
    assert(socket->domain == AF_UNIX);
    assert(addr);
    assert(addrlen);

    struct socket_connection connection;
    for (;;) {
        spin_lock(&socket->lock);
        if (socket->pending[0] != NULL) {
            memcpy(&connection, socket->pending[0], sizeof(struct socket_connection));

            free(socket->pending[0]);
            memmove(socket->pending, socket->pending + 1, (socket->pending_length - 1) * sizeof(struct socket_connection*));
            socket->pending[--socket->num_pending] = NULL;

            spin_unlock(&socket->lock);
            break;
        }

        spin_unlock(&socket->lock);
        yield();
        barrier();
    }

    // TODO: handle connection
    (void) connection;
    (void) addr;
    (void) addrlen;

    return -EAFNOSUPPORT;
}

int net_unix_bind(struct socket *socket, const struct sockaddr_un *addr, socklen_t addrlen) {
    assert(socket->domain == AF_UNIX);
    assert(addr);

    if (addr->sun_family != AF_UNIX) {
        return -EINVAL;
    }

    if (addrlen <= offsetof(struct sockaddr_un, sun_path) || addr->sun_path[0] != '/') {
        return -EINVAL;
    }

    struct unix_socket_data *data = calloc(1, sizeof(struct unix_socket_data));
    strncpy(data->bound_path, addr->sun_path, UNIX_PATH_MAX);

    if (iname(data->bound_path)) {
        free(data);
        return -EADDRINUSE;
    }

    int ret = fs_create(data->bound_path, S_IFSOCK | 0666);
    if (ret == -1) {
        free(data);
        return ret;
    }

    struct tnode *tnode = iname(data->bound_path);
    ret = fs_bind_socket_to_inode(tnode->inode, socket->id);
    if (ret == -1) {
        free(data);
        return ret;
    }

    socket->state = BOUND;
    socket->private_data = data;
    return 0;
}

int net_unix_close(struct socket *socket) {
    struct unix_socket_data *data = socket->private_data;
    if (socket->state == BOUND || socket->state == CONNECTED) {
        assert(data);

        struct tnode *tnode = iname(data->bound_path);
        assert(tnode);

        tnode->inode->socket_id = 0;
        free(data);
    }

    return 0;
}

int net_unix_connect(struct socket *socket, const struct sockaddr_un *addr, socklen_t addrlen) {
    assert(socket);
    assert(socket->domain == AF_UNIX);
    assert(addr);

    if (addr->sun_family != AF_UNIX) {
        return -EAFNOSUPPORT;
    }

    if (socket->state == CONNECTED) {
        return -EISCONN;
    }

    if (addrlen <= offsetof(struct sockaddr_un, sun_path) || addr->sun_path[0] != '/') {
        return -EINVAL;
    }

    struct tnode *tnode = iname(addr->sun_path);
    if (!tnode || tnode->inode->socket_id == 0) {
        return -ECONNREFUSED;
    }

    struct socket *connect_to = net_get_socket_by_id(tnode->inode->socket_id);
    assert(connect_to);

    spin_lock(&connect_to->lock);
    if (connect_to->state != LISTENING || connect_to->num_pending >= connect_to->pending_length) {
        spin_unlock(&connect_to->lock);
        return -ECONNREFUSED;
    }

    debug_log("Connectiing to socket: [ %lu ]\n", connect_to->id);

    struct socket_connection *connection = calloc(1, sizeof(struct socket_connection));
    connection->addr.un.sun_family = AF_UNIX;
    memcpy(connection->addr.un.sun_path, addr->sun_path, addrlen - offsetof(struct sockaddr_un, sun_path));

    connect_to->pending[connect_to->num_pending++] = connection;

    spin_unlock(&connect_to->lock);

    // TODO: handle connection
    return -EAFNOSUPPORT;
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