#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <kernel/fs/file.h>
#include <kernel/fs/vfs.h>
#include <kernel/hal/output.h>
#include <kernel/hal/processor.h>
#include <kernel/hal/timer.h>
#include <kernel/net/inet_socket.h>
#include <kernel/net/socket.h>
#include <kernel/net/tcp.h>
#include <kernel/net/unix_socket.h>
#include <kernel/proc/task.h>
#include <kernel/sched/task_sched.h>
#include <kernel/time/clock.h>
#include <kernel/util/hash_map.h>
#include <kernel/util/spinlock.h>

// #define SOCKET_DEBUG

static unsigned long socket_id_next = 1;
static spinlock_t id_lock = SPINLOCK_INITIALIZER;

static int socket_file_close(struct file *file);
static ssize_t net_read(struct file *file, off_t offset, void *buf, size_t len);
static ssize_t net_write(struct file *file, off_t offset, const void *buf, size_t len);

static struct list_node protocol_list = INIT_LIST(protocol_list);

static struct file_operations socket_file_ops = { .close = socket_file_close, .read = net_read, .write = net_write };

static struct hash_map *map;

HASH_DEFINE_FUNCTIONS(socket, struct socket, unsigned long, id)

static int socket_file_close(struct file *file) {
    assert(file);

    struct socket_file_data *file_data = file->private_data;
    assert(file_data);

    struct socket *socket = hash_get(map, &file_data->socket_id);
    assert(socket);

    mutex_lock(&socket->lock);

#ifdef SOCKET_DEBUG
    debug_log("Destroying socket: [ %lu ]\n", socket->id);
#endif /* SOCKET_DEBUG */

    int ret = 0;
    switch (socket->domain) {
        case AF_INET:
            ret = net_inet_close(socket);
            break;
        case AF_UNIX:
            ret = net_unix_close(socket);
            break;
        default:
            break;
    }

    struct socket_data *to_remove = socket->data_head;
    while (to_remove != NULL) {
        struct socket_data *next = to_remove->next;
        free(to_remove);
        to_remove = next;
    }

    hash_del(map, &socket->id);

    mutex_unlock(&socket->lock);

    free(socket);
    free(file_data);

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

struct socket *net_create_socket(int domain, int type, int protocol, struct socket_ops *op, int *fd, void *private_data) {
    struct task *current = get_current_task();

    for (int i = 0; i < FOPEN_MAX; i++) {
        if (current->process->files[i].file == NULL) {
            struct socket_file_data *file_data = malloc(sizeof(struct socket_file_data));
            current->process->files[i].file = fs_create_file(NULL, FS_SOCKET, FS_FILE_CANT_SEEK, O_RDWR, &socket_file_ops, file_data);
            current->process->files[i].fd_flags = (type & SOCK_CLOEXEC) ? FD_CLOEXEC : 0;

            spin_lock(&id_lock);
            file_data->socket_id = socket_id_next++;
            spin_unlock(&id_lock);

            struct socket *socket = calloc(1, sizeof(struct socket));
            socket->domain = domain;
            socket->type = type;
            socket->protocol = protocol;
            socket->id = file_data->socket_id;
            socket->timeout = (struct timeval) { 10, 0 };
            socket->writable = true;
            socket->readable = false;
            socket->exceptional = false;
            socket->has_host_address = false;
            socket->has_peer_address = false;
            socket->op = op;
            socket->private_data = private_data;
            init_mutex(&socket->lock);

            hash_put(map, socket);

            *fd = i;
            return socket;
        }
    }

    *fd = -EMFILE;
    return NULL;
}

ssize_t net_generic_recieve_from(struct socket *socket, void *buf, size_t len, struct sockaddr *addr, socklen_t *addrlen) {
    if (socket->state != CONNECTED && socket->type == SOCK_STREAM) {
        return -ENOTCONN;
    }

    struct socket_data *data;

    struct timespec start_time = time_read_clock(CLOCK_MONOTONIC);

    for (;;) {
        mutex_lock(&socket->lock);
        data = socket->data_head;

        if (data != NULL) {
            break;
        }

        mutex_unlock(&socket->lock);

        switch (socket->domain) {
            case AF_UNIX: {
                struct unix_socket_data *d = socket->private_data;
                if (!net_get_socket_by_id(d->connected_id)) {
#ifdef SOCKET_DEBUG
                    debug_log("Connection terminated: [ %lu ]\n", socket->id);
#endif /* SOCKET_DEBUG */
                    return 0;
                }
                break;
            }
            default:
                break;
        }

        if (socket->type & SOCK_NONBLOCK) {
            return -EAGAIN;
        }

        if (socket->timeout.tv_sec != 0 || socket->timeout.tv_usec != 0) {
            struct timespec timeout = time_from_timeval(socket->timeout);
            int ret = proc_block_until_socket_is_readable_with_timeout(get_current_task(), socket, time_add(timeout, start_time));
            if (ret) {
                return ret;
            }

            // We timed out
            if (time_compare(start_time, time_add(start_time, timeout)) >= 0) {
                return -EAGAIN;
            }

            continue;
        }

        int ret = proc_block_until_socket_is_readable(get_current_task(), socket);
        if (ret) {
            return ret;
        }
    }

    socket->data_head = data->next;
    remque(data);
    if (socket->data_head == NULL) {
        socket->data_tail = NULL;
        socket->readable = false;
    }

    if (socket->protocol == IPPROTO_TCP) {
        struct inet_socket_data *data = socket->private_data;
        assert(data);
        if (data->tcb->should_send_ack) {
            struct network_interface *interface = net_get_interface_for_ip(IP_V4_FROM_SOCKADDR(&socket->peer_address));

            net_send_tcp(interface, IP_V4_FROM_SOCKADDR(&socket->peer_address), PORT_FROM_SOCKADDR(&socket->host_address),
                         PORT_FROM_SOCKADDR(&socket->peer_address), data->tcb->current_sequence_num, data->tcb->current_ack_num,
                         (union tcp_flags) { .bits.ack = 1, .bits.fin = socket->state == CLOSING }, 0, NULL);
            data->tcb->should_send_ack = false;

            if (socket->state == CLOSING) {
                socket->state = CLOSED;
            }
        }
    }

    mutex_unlock(&socket->lock);

    size_t to_copy = MIN(len, data->len);
    memcpy(buf, data->data, to_copy);

    if (addr && addrlen) {
        size_t len = MIN(data->from.addrlen, *addrlen);
        memcpy(addr, &data->from.addr, len);
        *addrlen = len;
    }

#ifdef SOCKET_DEBUG
    debug_log("Received message: [ %lu, %lu ]\n", socket->id, to_copy);
#endif /* SOCKET_DEBUG */

    free(data);
    return (ssize_t) to_copy;
}

int net_get_next_connection(struct socket *socket, struct socket_connection *connection) {
    for (;;) {
        mutex_lock(&socket->lock);
        if (socket->pending[0] != NULL) {
            memcpy(connection, socket->pending[0], sizeof(struct socket_connection));

            free(socket->pending[0]);
            memmove(socket->pending, socket->pending + 1, (socket->pending_length - 1) * sizeof(struct socket_connection *));
            socket->pending[--socket->num_pending] = NULL;

            if (socket->num_pending == 0) {
                socket->readable = false;
            }

            mutex_unlock(&socket->lock);
            break;
        }

        mutex_unlock(&socket->lock);

        if (socket->type & SOCK_NONBLOCK) {
            return -EAGAIN;
        }

        int ret = proc_block_until_socket_is_readable(get_current_task(), socket);
        if (ret) {
            return ret;
        }
    }

    return 0;
}

struct socket *net_get_socket_by_id(unsigned long id) {
    return hash_get(map, &id);
}

void net_for_each_socket(void (*f)(struct socket *socket, void *data), void *data) {
    hash_for_each(map, (void (*)(void *, void *)) f, data);
}

ssize_t net_send_to_socket(struct socket *to_send, struct socket_data *socket_data) {
    mutex_lock(&to_send->lock);
    insque(socket_data, to_send->data_tail);
    if (!to_send->data_head) {
        to_send->data_head = to_send->data_tail = socket_data;
    } else {
        to_send->data_tail = socket_data;
    }

    to_send->readable = true;

#ifdef SOCKET_DEBUG
    debug_log("Sent message to: [ %lu ]\n", to_send->id);
#endif /* SOCKET_DEBUG */

    ssize_t ret = socket_data->len;
    mutex_unlock(&to_send->lock);
    return ret;
}

void net_set_host_address(struct socket *socket, const void *addr, socklen_t addrlen) {
    assert(addrlen <= sizeof(struct sockaddr_storage));
    memcpy(&socket->host_address, addr, addrlen);
    socket->has_host_address = true;
}

void net_set_peer_address(struct socket *socket, const void *addr, socklen_t addrlen) {
    assert(addrlen <= sizeof(struct sockaddr_storage));
    memcpy(&socket->peer_address, addr, addrlen);
    socket->has_peer_address = true;
}

void net_copy_sockaddr_to_user(const void *addr, size_t addrlen, void *user_addr, socklen_t *user_addrlen) {
    size_t len_to_copy = MIN(*user_addrlen, addrlen);
    memcpy(user_addr, addr, len_to_copy);
    *user_addrlen = addrlen;
}

int net_accept(struct file *file, struct sockaddr *addr, socklen_t *addrlen, int flags) {
    assert(file);
    assert(file->private_data);

    struct socket_file_data *file_data = file->private_data;
    struct socket *socket = hash_get(map, &file_data->socket_id);
    assert(socket);

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
    assert(file);
    assert(file->private_data);

    struct socket_file_data *file_data = file->private_data;
    struct socket *socket = hash_get(map, &file_data->socket_id);
    assert(socket);

    if (!socket->op->bind) {
        return -EINVAL;
    }

    return socket->op->bind(socket, addr, addrlen);
}

int net_connect(struct file *file, const struct sockaddr *addr, socklen_t addrlen) {
    assert(file);
    assert(file->private_data);

    struct socket_file_data *file_data = file->private_data;
    struct socket *socket = hash_get(map, &file_data->socket_id);
    assert(socket);

    if (!socket->op->connect) {
        return -EINVAL;
    }

    return socket->op->connect(socket, addr, addrlen);
}

int net_getpeername(struct file *file, struct sockaddr *addr, socklen_t *addrlen) {
    assert(file);
    assert(file->private_data);

    struct socket_file_data *file_data = file->private_data;
    struct socket *socket = hash_get(map, &file_data->socket_id);
    assert(socket);

    if (!socket->op->getpeername) {
        return -EINVAL;
    }

    return socket->op->getpeername(socket, addr, addrlen);
}

int net_listen(struct file *file, int backlog) {
    assert(file);
    assert(file->private_data);

    struct socket_file_data *file_data = file->private_data;
    struct socket *socket = hash_get(map, &file_data->socket_id);
    assert(socket);

    if (backlog <= 0 || socket->state != BOUND) {
        return -EINVAL;
    }

    switch (socket->domain) {
        case AF_INET: {
            int ret = net_inet_listen(socket);
            if (ret < 0) {
                return ret;
            }
            break;
        }
        case AF_UNIX:
            break;
        default:
            return -EAFNOSUPPORT;
    }

    socket->pending = calloc(backlog, sizeof(struct socket_connection *));
    socket->pending_length = backlog;
    socket->num_pending = 0;

#ifdef SOCKET_DEBUG
    debug_log("Set socket to listening: [ %lu, %d ]\n", socket->id, socket->pending_length);
#endif /* SOCKET_DEBUG */

    socket->state = LISTENING;
    return 0;
}

int net_setsockopt(struct file *file, int level, int optname, const void *optval, socklen_t optlen) {
    assert(file);
    assert(file->private_data);

    struct socket_file_data *file_data = file->private_data;
    struct socket *socket = hash_get(map, &file_data->socket_id);
    assert(socket);

    if (level != SOL_SOCKET) {
        return -ENOPROTOOPT;
    }

    if (optname != SO_RCVTIMEO) {
        return -ENOPROTOOPT;
    }

    if (optlen != sizeof(struct timeval)) {
        return -EINVAL;
    }

    socket->timeout = *((const struct timeval *) optval);

    return 0;
}

int net_socket(int domain, int type, int protocol) {
    if (type == SOCK_RAW && get_current_task()->process->euid != 0) {
        return -EACCES;
    }

    bool saw_af_family = false;
    list_for_each_entry(&protocol_list, iter, struct socket_protocol, list) {
        if (iter->domain == domain) {
            if (iter->type == (type & SOCK_TYPE_MASK) && (iter->protocol == protocol || (protocol == 0 && iter->is_default_protocol))) {
                return iter->create_socket(domain, type, iter->protocol);
            }

            saw_af_family = true;
        }
    }

    return saw_af_family ? -EPROTONOSUPPORT : -EAFNOSUPPORT;
}

ssize_t net_sendto(struct file *file, const void *buf, size_t len, int flags, const struct sockaddr *dest, socklen_t addrlen) {
    assert(file);
    assert(file->private_data);

    struct socket_file_data *file_data = file->private_data;
    struct socket *socket = hash_get(map, &file_data->socket_id);
    assert(socket);

    if (!socket->op->sendto) {
        return -EINVAL;
    }

    return socket->op->sendto(socket, buf, len, flags, dest, addrlen);
}

ssize_t net_recvfrom(struct file *file, void *buf, size_t len, int flags, struct sockaddr *source, socklen_t *addrlen) {
    assert(file);
    assert(file->private_data);

    struct socket_file_data *file_data = file->private_data;
    struct socket *socket = hash_get(map, &file_data->socket_id);
    assert(socket);

    if (!socket->op->recvfrom) {
        return -EINVAL;
    }

    return socket->op->recvfrom(socket, buf, len, flags, source, addrlen);
}

void net_register_protocol(struct socket_protocol *protocol) {
    list_append(&protocol_list, &protocol->list);
    debug_log("Registered socket protocol: [ %s ]\n", protocol->name);
}

void init_net_sockets() {
    map = hash_create_hash_map(socket_hash, socket_equals, socket_key);
    init_unix_sockets();
    init_inet_sockets();
}
