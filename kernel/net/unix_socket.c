#include <errno.h>
#include <search.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <kernel/fs/vfs.h>
#include <kernel/hal/processor.h>
#include <kernel/net/socket.h>
#include <kernel/net/socket_syscalls.h>
#include <kernel/net/unix_socket.h>
#include <kernel/proc/task.h>
#include <kernel/sched/task_sched.h>
#include <kernel/util/init.h>

// #define UNIX_DEBUG

#define PATH_FROM_SOCKADDR(s) (((struct sockaddr_un *) (s))->sun_path)

static int net_unix_accept(struct socket *socket, struct sockaddr *addr, socklen_t *addrlen, int flags);
static int net_unix_bind(struct socket *socket, const struct sockaddr *addr, socklen_t addrlen);
static int net_unix_close(struct socket *socket);
static int net_unix_connect(struct socket *socket, const struct sockaddr *addr, socklen_t addrlen);
static int net_unix_getpeername(struct socket *socket, struct sockaddr *addr, socklen_t *addrlen);
static int net_unix_getsockname(struct socket *socket, struct sockaddr *addr, socklen_t *addrlen);
static int net_unix_socket(int domain, int type, int protocol);
static int net_unix_socketpair(int domain, int type, int protocol, int *fds);
static ssize_t net_unix_sendto(struct socket *socket, const void *buf, size_t len, int flags, const struct sockaddr *dest,
                               socklen_t addrlen);

static struct socket_ops unix_ops = {
    .accept = net_unix_accept,
    .bind = net_unix_bind,
    .close = net_unix_close,
    .connect = net_unix_connect,
    .getpeername = net_unix_getpeername,
    .getsockname = net_unix_getsockname,
    .getsockopt = net_generic_getsockopt,
    .setsockopt = net_generic_setsockopt,
    .listen = net_generic_listen,
    .recvfrom = net_generic_recieve_from,
    .sendto = net_unix_sendto,
};

static int net_unix_accept(struct socket *socket, struct sockaddr *addr, socklen_t *addrlen, int flags) {
    struct socket_connection connection;
    int ret = net_get_next_connection(socket, &connection);
    if (ret != 0) {
        return ret;
    }

#ifdef UNIX_DEBUG
    debug_log("Creating connection: [ %lu, %lu ]\n", socket->id, connection.connect_to_id);
#endif /* UNIX_DEBUG */

    int fd = 0;
    struct socket *new_socket =
        net_create_socket_fd(socket->domain, (socket->type & SOCK_TYPE_MASK) | flags, socket->protocol, &unix_ops, &fd, NULL);
    if (new_socket == NULL) {
        return fd;
    }

    if (addr) {
        net_copy_sockaddr_to_user(&connection.addr.un, sizeof(struct sockaddr_un), addr, addrlen);
    }

    net_set_host_address(new_socket, &socket->host_address, sizeof(struct sockaddr_un));

    struct socket *connect_to = connection.connect_to;
    assert(connect_to);
    new_socket->private_data = net_bump_socket(connect_to);

    mutex_lock(&connect_to->lock);
    connect_to->private_data = net_bump_socket(new_socket);
    net_set_peer_address(new_socket, &socket->host_address, sizeof(struct sockaddr_un));
    connect_to->state = CONNECTED;
    mutex_unlock(&connect_to->lock);

    new_socket->state = CONNECTED;
    return fd;
}

static int net_unix_bind(struct socket *socket, const struct sockaddr *addr, socklen_t addrlen) {
    if (addrlen <= offsetof(struct sockaddr_un, sun_path) || PATH_FROM_SOCKADDR(addr)[0] != '/' ||
        addrlen > sizeof(struct sockaddr_storage)) {
        return -EINVAL;
    }

    net_set_host_address(socket, addr, addrlen);
    char *path = PATH_FROM_SOCKADDR(&socket->host_address);

    // Ensure null termination, so that the iname call will be bounded.
    path[UNIX_PATH_MAX - 1] = '\0';

    if (iname(path, 0, NULL) == 0) {
        return -EADDRINUSE;
    }

    int ret = 0;
    struct tnode *tnode = fs_mknod(path, S_IFSOCK | 0666, 0, &ret);
    if (ret < 0) {
        return ret;
    }

    struct inode *inode = tnode->inode;
    drop_tnode(tnode);
    ret = fs_bind_socket_to_inode(inode, socket);
    if (ret == -1) {
        return ret;
    }

    socket->state = BOUND;
    return 0;
}

static int net_unix_close(struct socket *socket) {
    if (socket->state == BOUND || socket->state == LISTENING) {
        struct tnode *tnode;
        assert(iname(PATH_FROM_SOCKADDR(&socket->host_address), 0, &tnode) == 0);
        assert(tnode);

        tnode->inode->socket = NULL;
    }

    if (socket->state == CONNECTED) {
        struct socket *connected_to = socket->private_data;
        if (connected_to) {
            // We terminated the connection
            fs_trigger_state(&connected_to->file_state, POLLIN);
            connected_to->state = CLOSED;
            net_drop_socket(connected_to);
        }
    }

    return 0;
}

static int net_unix_connect(struct socket *socket, const struct sockaddr *addr, socklen_t addrlen) {
    if (socket->state == CONNECTED) {
        return -EISCONN;
    }

    const char *path = PATH_FROM_SOCKADDR(addr);
    if (addrlen <= offsetof(struct sockaddr_un, sun_path) || path[0] != '/') {
        return -EINVAL;
    }

    struct tnode *tnode = NULL;
    int ret = iname(path, 0, &tnode);
    if (ret < 0) {
        return ret;
    }

    struct inode *inode = tnode->inode;
    drop_tnode(tnode);

    if (!fs_can_write_inode(inode)) {
        return -EACCES;
    }

    if (inode->socket == NULL) {
        // There is no socket bound to this inode
        return -ECONNREFUSED;
    }

    struct socket *connect_to = inode->socket;
    assert(connect_to);

    mutex_lock(&connect_to->lock);
    if (connect_to->state != LISTENING || connect_to->num_pending >= connect_to->pending_length) {
        mutex_unlock(&connect_to->lock);
        return -ECONNREFUSED;
    }

#ifdef UNIX_DEBUG
    debug_log("Connecting to socket: [ %lu ]\n", connect_to->id);
#endif /* UNIX_DEBUG */

    struct socket_connection *connection = calloc(1, sizeof(struct socket_connection));
    connection->addr.un.sun_family = AF_UNIX;
    memcpy(connection->addr.un.sun_path, path, addrlen - offsetof(struct sockaddr_un, sun_path));
    connection->addrlen = addrlen;
    connection->connect_to = socket;

    connect_to->pending[connect_to->num_pending++] = connection;
    fs_trigger_state(&connect_to->file_state, POLLIN);

    mutex_lock(&socket->lock);
    mutex_unlock(&connect_to->lock);

    for (;;) {
        enum socket_state state = socket->state;
        if (state == CONNECTED) {
            break;
        }

        int ret = net_poll_wait(socket, POLLIN, NULL);
        if (ret) {
            return ret;
        }
    }

    mutex_unlock(&socket->lock);
    return 0;
}

static int net_unix_getpeername(struct socket *socket, struct sockaddr *addr, socklen_t *addrlen) {
    int ret = 0;

    mutex_lock(&socket->lock);
    if (socket->has_peer_address) {
        struct sockaddr_un *peer_address = (struct sockaddr_un *) &socket->peer_address;
        size_t peer_address_length = offsetof(struct sockaddr_un, sun_path) + strnlen(peer_address->sun_path, UNIX_PATH_MAX - 1) + 1;
        net_copy_sockaddr_to_user(peer_address, peer_address_length, addr, addrlen);
    } else {
        ret = -ENOTCONN;
    }
    mutex_unlock(&socket->lock);

    return ret;
}

static int net_unix_getsockname(struct socket *socket, struct sockaddr *addr, socklen_t *addrlen) {
    int ret = 0;

    mutex_lock(&socket->lock);
    if (socket->has_host_address) {
        struct sockaddr_un *host_address = (struct sockaddr_un *) &socket->host_address;
        size_t host_address_length = offsetof(struct sockaddr_un, sun_path) + strnlen(host_address->sun_path, UNIX_PATH_MAX - 1) + 1;
        net_copy_sockaddr_to_user(host_address, host_address_length, addr, addrlen);
    } else {
        ret = -ENOTCONN;
    }
    mutex_unlock(&socket->lock);

    return ret;
}

static int net_unix_socket(int domain, int type, int protocol) {
    int fd;
    net_create_socket_fd(domain, type, protocol, &unix_ops, &fd, NULL);
    return fd;
}

static int net_unix_socketpair(int domain, int type, int protocol, int *fds) {
    int fd1;
    struct socket *s1 = net_create_socket_fd(domain, type, protocol, &unix_ops, &fd1, NULL);
    if (!s1) {
        return fd1;
    }

    int fd2;
    struct socket *s2 = net_create_socket_fd(domain, type, protocol, &unix_ops, &fd2, net_bump_socket(s1));
    if (!s2) {
        net_drop_socket(s2);
        fs_close(get_current_process()->files[fd1].file);
        get_current_process()->files[fd1].file = NULL;
        return fd2;
    }
    s2->private_data = net_bump_socket(s1);

    s1->state = CONNECTED;
    s2->state = CONNECTED;

    fds[0] = fd1;
    fds[1] = fd2;
    return 0;
}

static ssize_t net_unix_sendto(struct socket *socket, const void *buf, size_t len, int flags, const struct sockaddr *dest,
                               socklen_t addrlen) {
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
    struct socket *to_send = socket->private_data;
    if (to_send == NULL) {
        free(socket_data);
        return -ECONNABORTED;
    }

    if (socket->has_host_address) {
        const char *path = PATH_FROM_SOCKADDR(&socket->host_address);
        socket_data->from.addrlen = offsetof(struct sockaddr_un, sun_path) + strnlen(path, UNIX_PATH_MAX) + 1;
        memcpy(&socket_data->from.addr.un, &socket->host_address, socket_data->from.addrlen);
    }

    return net_send_to_socket(to_send, socket_data);
}

static struct socket_protocol unix_stream_protocol = {
    .domain = AF_UNIX,
    .type = SOCK_STREAM,
    .protocol = 0,
    .is_default_protocol = true,
    .name = "Unix Stream",
    .create_socket = net_unix_socket,
    .create_socket_pair = net_unix_socketpair,
};

static void init_unix_sockets(void) {
    net_register_protocol(&unix_stream_protocol);
}
INIT_FUNCTION(init_unix_sockets, net);
