#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <kernel/fs/file.h>
#include <kernel/hal/output.h>
#include <kernel/proc/process.h>
#include <kernel/net/socket.h>
#include <kernel/net/unix_socket.h>
#include <kernel/util/hash_map.h>
#include <kernel/util/spinlock.h>

static unsigned long socket_id_next = 1;
static spinlock_t id_lock = SPINLOCK_INITIALIZER;

static int socket_file_close(struct file *file);
static ssize_t net_read(struct file *file, void *buf, size_t len);
static ssize_t net_write(struct file *file, const void *buf, size_t len);

static struct file_operations socket_file_ops = { 
    socket_file_close, net_read, net_write, NULL
};

static struct hash_map *map;

static int socket_hash(void *i, int num_buckets) {
    return *((unsigned long*) i) % num_buckets;
}

static int socket_equals(void *i1, void *i2) {
    return *((unsigned long*) i1) == *((unsigned long*) i2);
}

static void *socket_key(void *socket) {
    return &((struct socket*) socket)->id;
}

static int socket_file_close(struct file *file) {
    assert(file);

    struct socket_file_data *file_data = file->private_data;
    assert(file_data);

    struct socket *socket = hash_get(map, &file_data->socket_id);
    assert(socket);

    debug_log("Destroying socket: [ %lu ]\n", socket->id);

    int ret = 0;
    switch (socket->domain) {
        case AF_UNIX:
            ret = net_unix_close(socket);
            break;
        default:
            break;
    }

    hash_del(map, &socket->id);
    free(socket);
    free(file_data);

    return ret;
}

static ssize_t net_read(struct file *file, void *buf, size_t len) {
    assert(file);
    assert(file->private_data);

    struct socket_file_data *file_data = file->private_data;
    struct socket *socket = hash_get(map, &file_data->socket_id);
    assert(socket);

    switch (socket->domain) {
        case AF_UNIX:
            return net_unix_recv(socket, buf, len);
        default:
            return -EAFNOSUPPORT;
    }
}

static ssize_t net_write(struct file *file, const void *buf, size_t len) {
    assert(file);
    assert(file->private_data);

    struct socket_file_data *file_data = file->private_data;
    struct socket *socket = hash_get(map, &file_data->socket_id);
    assert(socket);

    switch (socket->domain) {
        case AF_UNIX:
            return net_unix_send(socket, buf, len);
        default:
            return -EAFNOSUPPORT;
    }
}

struct socket *net_create_socket(int domain, int type, int protocol, int *fd) {
    struct process *current = get_current_process();

    for (int i = 0; i < FOPEN_MAX; i++) {
        if (current->files[i] == NULL) {
            current->files[i] = calloc(1, sizeof(struct file));
            current->files[i]->flags = FS_SOCKET;
            current->files[i]->f_op = &socket_file_ops;

            struct socket_file_data *file_data = malloc(sizeof(struct socket_file_data));
            current->files[i]->private_data = file_data;

            spin_lock(&id_lock);
            file_data->socket_id = socket_id_next++;
            spin_unlock(&id_lock);

            struct socket *socket = calloc(1, sizeof(struct socket));
            socket->domain = domain;
            socket->type = type;
            socket->protocol = protocol;
            socket->id = file_data->socket_id;
            init_spinlock(&socket->lock);

            hash_put(map, socket);

            *fd = i;
            return socket;
        }
    }

    *fd = -EMFILE;
    return NULL;
}

int net_accept(struct file *file, struct sockaddr *addr, socklen_t *addrlen) {
    assert(file);
    assert(file->private_data);

    struct socket_file_data *file_data = file->private_data;
    struct socket *socket = hash_get(map, &file_data->socket_id);
    assert(socket);

    if (socket->state != LISTENING) {
        return -EINVAL;
    }

    if (socket->type != SOCK_STREAM) {
        return -EOPNOTSUPP;
    }

    switch (socket->domain) {
        case AF_UNIX:
            return net_unix_accept(socket, (struct sockaddr_un*) addr, addrlen);
        default:
            return -EAFNOSUPPORT;
    }
}

int net_bind(struct file *file, const struct sockaddr *addr, socklen_t addrlen) {
    assert(file);
    assert(file->private_data);

    struct socket_file_data *file_data = file->private_data;
    struct socket *socket = hash_get(map, &file_data->socket_id);
    assert(socket);

    switch (socket->domain) {
        case AF_UNIX:
            return net_unix_bind(socket, (const struct sockaddr_un*) addr, addrlen);
        default:
            return -EAFNOSUPPORT;
    }
}

int net_connect(struct file *file, const struct sockaddr *addr, socklen_t addrlen) {
    assert(file);
    assert(file->private_data);

    struct socket_file_data *file_data = file->private_data;
    struct socket *socket = hash_get(map, &file_data->socket_id);
    assert(socket);

    switch (socket->domain) {
        case AF_UNIX:
            return net_unix_connect(socket, (const struct sockaddr_un*) addr, addrlen);
        default:
            return -EAFNOSUPPORT;
    }
}

int net_listen(struct file *file, int backlog) {
    assert(file);
    assert(file->private_data);

    struct socket_file_data *file_data = file->private_data;
    struct socket *socket = hash_get(map, &file_data->socket_id);
    assert(socket);

    if (backlog <= 0) {
        return -EINVAL;
    }

    switch (socket->domain) {
        case AF_UNIX:
            break;
        default:
            return -EAFNOSUPPORT;
    }

    socket->pending = calloc(backlog, sizeof(struct socket_connection*));
    socket->pending_length = backlog;

    socket->state = LISTENING;
    return 0;
}

int net_socket(int domain, int type, int protocol) {
    switch (domain) {
        case AF_UNIX:
            return net_unix_socket(domain, type, protocol);
        default:
            return -EAFNOSUPPORT;
    }
}

struct socket *net_get_socket_by_id(unsigned long id) {
    return hash_get(map, &id);
}

void init_net_sockets() {
    map = hash_create_hash_map(socket_hash, socket_equals, socket_key);
}