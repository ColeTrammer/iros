#include <errno.h>
#include <search.h>
#include <string.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <kernel/fs/vfs.h>
#include <kernel/net/socket.h>
#include <kernel/net/unix_socket.h>
#include <kernel/proc/process.h>
#include <kernel/sched/process_sched.h>

int net_unix_accept(struct socket *socket, struct sockaddr_un *addr, socklen_t *addrlen, int flags) {
    assert(socket->domain == AF_UNIX);
    assert(socket->state == LISTENING);
    assert(socket->private_data);
    assert(addr);
    assert(addrlen);

    struct socket_connection connection;
    int ret = net_get_next_connection(socket, &connection);
    if (ret != 0) {
        return ret;
    }

    debug_log("Creating connection: [ %lu, %lu ]\n", socket->id, connection.connect_to_id);

    int fd = 0;
    struct socket *new_socket = net_create_socket(socket->domain, (socket->type & SOCK_TYPE_MASK) | flags, socket->protocol, &fd);
    if (new_socket == NULL) {
        return fd;
    }

    memcpy(addr, &connection.addr, *addrlen);
    *addrlen = connection.addrlen;

    struct unix_socket_data *new_data = calloc(1, sizeof(struct unix_socket_data));
    new_socket->private_data = new_data;
    memcpy(new_data->bound_path, connection.addr.un.sun_path, connection.addrlen - offsetof(struct sockaddr_un, sun_path));
    new_data->connected_id = connection.connect_to_id;

    struct socket *connect_to = net_get_socket_by_id(new_data->connected_id);
    assert(connect_to);

    spin_lock(&connect_to->lock);

    assert(connect_to->private_data);
    struct unix_socket_data *connect_to_data = connect_to->private_data;
    memcpy(connect_to_data->bound_path, ((struct unix_socket_data*) socket->private_data)->bound_path, sizeof(connect_to_data->bound_path));
    connect_to_data->connected_id = new_socket->id;

    connect_to->state = CONNECTED;
    spin_unlock(&connect_to->lock);

    new_socket->state = CONNECTED;
    return fd;
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
    if (socket->state == BOUND || socket->state == LISTENING) {
        assert(data);

        struct tnode *tnode = iname(data->bound_path);
        assert(tnode);

        tnode->inode->socket_id = 0;
    }

    free(data);
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

    debug_log("Connecting to socket: [ %lu ]\n", connect_to->id);

    struct socket_connection *connection = calloc(1, sizeof(struct socket_connection));
    connection->addr.un.sun_family = AF_UNIX;
    memcpy(connection->addr.un.sun_path, addr->sun_path, addrlen - offsetof(struct sockaddr_un, sun_path));
    connection->addrlen = addrlen;
    connection->connect_to_id = socket->id;

    connect_to->pending[connect_to->num_pending++] = connection;

    spin_lock(&socket->lock);
    spin_unlock(&connect_to->lock);

    socket->private_data = calloc(1, sizeof(struct unix_socket_data));
    assert(socket->private_data);
    spin_unlock(&socket->lock);

    for (;;) {
        if (socket->state == CONNECTED) {
            break;
        }

        yield();
        barrier();
    }

    return 0;
}

int net_unix_socket(int domain, int type, int protocol) {
    if ((type & SOCK_TYPE_MASK) != SOCK_STREAM || protocol != 0) {
        return -EPROTONOSUPPORT;
    }

    int fd;
    struct socket *socket = net_create_socket(domain, type, protocol, &fd);
    (void) socket;

    return fd;
}

ssize_t net_unix_recvfrom(struct socket *socket, void *buf, size_t len, int flags, struct sockaddr_un *source, socklen_t *addrlen) {
    assert(socket->domain == AF_UNIX);
    assert(buf);

    (void) flags;

    return net_generic_recieve_from(socket, buf, len, (struct sockaddr*) source, addrlen);
}

ssize_t net_unix_sendto(struct socket *socket, const void *buf, size_t len, int flags, const struct sockaddr_un *dest, socklen_t addrlen) {
    assert(socket);
    assert(socket->domain == AF_UNIX);
    assert(buf);

    (void) flags;

    if (dest) {
        net_unix_connect(socket, dest, addrlen);
    }

    if (socket->state != CONNECTED) {
        return -ENOTCONN;
    }

    struct socket_data *socket_data = malloc(sizeof(struct socket_data) + len);
    memcpy(socket_data->data, buf, len);
    socket_data->len = len;

    // FIXME: this seems very prone to data races when connections close
    struct unix_socket_data *data = socket->private_data;
    assert(data);

    struct socket *to_send = net_get_socket_by_id(data->connected_id);
    if (to_send == NULL) {
        free(socket_data);
        return -ECONNABORTED;
    }

    socket_data->from.addrlen = offsetof(struct sockaddr_un, sun_path) + strlen(data->bound_path) + 1;
    memcpy(&socket_data->from.addr, data->bound_path, socket_data->from.addrlen - offsetof(struct sockaddr_un, sun_path));
    socket_data->from.addr.un.sun_family = AF_UNIX;

    return net_send_to_socket(to_send, socket_data);
}