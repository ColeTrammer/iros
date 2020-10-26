#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <net/if.h>
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
#include <kernel/net/tcp_socket.h>
#include <kernel/net/unix_socket.h>
#include <kernel/proc/task.h>
#include <kernel/sched/task_sched.h>
#include <kernel/time/clock.h>
#include <kernel/util/hash_map.h>
#include <kernel/util/spinlock.h>

// #define SOCKET_DEBUG

static struct list_node protocol_list = INIT_LIST(protocol_list);
static struct list_node socket_list = INIT_LIST(socket_list);

struct list_node *net_get_socket_list(void) {
    return &socket_list;
}

struct socket *net_create_socket(int domain, int type, int protocol, struct socket_ops *op, void *private_data) {
    struct socket *socket = calloc(1, sizeof(struct socket));
    socket->domain = domain;
    socket->type = type;
    socket->protocol = protocol;
    socket->ref_count = 1;
    socket->recv_timeout = (struct timeval) { 10, 0 };
    socket->send_timeout = (struct timeval) { 10, 0 };
    init_file_state(&socket->file_state, false, type == SOCK_DGRAM || type == SOCK_RAW);
    socket->has_host_address = false;
    socket->has_peer_address = false;
    socket->op = op;
    socket->private_data = private_data;
    init_mutex(&socket->lock);
    list_append(&socket_list, &socket->socket_list);
    return socket;
}

static void net_destroy_socket(struct socket *socket) {
    if (socket->op->destroy) {
        socket->op->destroy(socket);
    }

    for (int i = 0; i < socket->num_pending; i++) {
        free(socket->pending[i]);
    }
    free(socket->pending);

    struct socket_data *to_remove = socket->data_head;
    while (to_remove != NULL) {
        struct socket_data *next = to_remove->next;
        free(to_remove);
        to_remove = next;
    }

    list_remove(&socket->socket_list);
    free(socket);
}

struct socket *net_bump_socket(struct socket *socket) {
    atomic_fetch_add(&socket->ref_count, 1);
    return socket;
}

void net_drop_socket(struct socket *socket) {
    int fetched_ref_count = atomic_fetch_sub(&socket->ref_count, 1);
    if (fetched_ref_count == 1) {
        net_destroy_socket(socket);
    }
}

int net_block_until_socket_is_readable(struct socket *socket) {
    if (socket->recv_timeout.tv_sec != 0 || socket->recv_timeout.tv_usec != 0) {
        struct timespec timeout = time_from_timeval(socket->recv_timeout);
        int ret = net_poll_wait(socket, POLLIN | POLLERR, &timeout);
        if (ret) {
            return ret;
        }

        if (timeout.tv_sec == 0 && timeout.tv_nsec == 0) {
            mutex_unlock(&socket->lock);
            return -EAGAIN;
        }

        return 0;
    }

    return net_poll_wait(socket, POLLIN | POLLERR, NULL);
}

int net_block_until_socket_is_writable(struct socket *socket) {
    if (socket->send_timeout.tv_sec != 0 || socket->send_timeout.tv_usec != 0) {
        struct timespec timeout = time_from_timeval(socket->send_timeout);
        int ret = net_poll_wait(socket, POLLOUT, &timeout);
        if (ret) {
            return ret;
        }

        if (timeout.tv_sec == 0 && timeout.tv_nsec == 0) {
            mutex_unlock(&socket->lock);
            return -EAGAIN;
        }

        return 0;
    }

    return net_poll_wait(socket, POLLOUT, NULL);
}

struct socket_data *net_get_next_message(struct socket *socket, int *error) {
    if (socket->state != CONNECTED && socket->type == SOCK_STREAM) {
        *error = -ENOTCONN;
        return NULL;
    }

    struct socket_data *data;
    mutex_lock(&socket->lock);
    for (;;) {
        data = socket->data_head;

        if (data != NULL) {
            break;
        }

        if (socket->error != 0) {
            *error = -socket->error;
            socket->error = 0;
            mutex_unlock(&socket->lock);
            return NULL;
        }

        bool connection_terminated = socket->state == CLOSED;
        if (connection_terminated) {
            mutex_unlock(&socket->lock);
            *error = -ECONNRESET;
            return NULL;
        }

        if (socket->type & SOCK_NONBLOCK) {
            mutex_unlock(&socket->lock);
            *error = -EAGAIN;
            return NULL;
        }

        int ret = net_block_until_socket_is_readable(socket);
        if (ret) {
            *error = ret;
            return NULL;
        }
    }

    socket->data_head = data->next;
    remque(data);
    if (socket->data_head == NULL) {
        socket->data_tail = NULL;
        fs_detrigger_state(&socket->file_state, POLLIN);
    }

    mutex_unlock(&socket->lock);
    return data;
}

ssize_t net_generic_recieve_from(struct socket *socket, void *buf, size_t len, int flags, struct sockaddr *addr, socklen_t *addrlen) {
    (void) flags;

    int error = 0;
    struct socket_data *data = net_get_next_message(socket, &error);
    if (error) {
        return error;
    }

    size_t to_copy = MIN(len, data->len);
    memcpy(buf, data->data, to_copy);

    if (addr && addrlen) {
        net_copy_sockaddr_to_user(&data->from.addr, data->from.addrlen, addr, addrlen);
    }

#ifdef SOCKET_DEBUG
    debug_log("Received message: [ %lu, %lu ]\n", socket->id, to_copy);
#endif /* SOCKET_DEBUG */

    free(data);
    return (ssize_t) to_copy;
}

int net_generic_listen(struct socket *socket, int backlog) {
    socket->pending = calloc(backlog, sizeof(struct socket_connection *));
    socket->pending_length = backlog;
    socket->num_pending = 0;

#ifdef SOCKET_DEBUG
    debug_log("Set socket to listening: [ %lu, %d ]\n", socket->id, socket->pending_length);
#endif /* SOCKET_DEBUG */

    socket->state = LISTENING;
    return 0;
}

int net_generic_setsockopt(struct socket *socket, int level, int optname, const void *optval, socklen_t optlen) {
    if (level != SOL_SOCKET) {
        return -ENOPROTOOPT;
    }

    switch (optname) {
        case SO_ACCEPTCONN:
            return -ENOPROTOOPT;
        case SO_BROADCAST: {
            int value = NET_READ_SOCKOPT(int, optval, optlen);
            socket->broadcast = !!value;
            return 0;
        }
        case SO_DEBUG: {
            int value = NET_READ_SOCKOPT(int, optval, optlen);
            socket->debug = !!value;
            return 0;
        }
        case SO_DONTROUTE: {
            int value = NET_READ_SOCKOPT(int, optval, optlen);
            socket->dont_route = !!value;
            return 0;
        }
        case SO_ERROR:
            return -ENOPROTOOPT;
        case SO_KEEPALIVE: {
            int value = NET_READ_SOCKOPT(int, optval, optlen);
            socket->keepalive = !!value;
            return 0;
        }
        case SO_LINGER: {
            struct linger value = NET_READ_SOCKOPT(struct linger, optval, optlen);
            socket->linger = value;
            return 0;
        }
        case SO_OOBINLINE: {
            int value = NET_READ_SOCKOPT(int, optval, optlen);
            socket->oob_inline = !!value;
            return 0;
        }
        case SO_RCVBUF: {
            int value = NET_READ_SOCKOPT(int, optval, optlen);
            socket->recv_buffer_max = value;
            return 0;
        }
        case SO_RCVLOWAT: {
            int value = NET_READ_SOCKOPT(int, optval, optlen);
            socket->recv_low_water_mark = value;
            return 0;
        }
        case SO_RCVTIMEO: {
            struct timeval value = NET_READ_SOCKOPT(struct timeval, optval, optlen);
            socket->recv_timeout = value;
            return 0;
        }
        case SO_REUSEADDR: {
            int value = NET_READ_SOCKOPT(int, optval, optlen);
            socket->reuse_addr = !!value;
            return 0;
        }
        case SO_SNDBUF: {
            int value = NET_READ_SOCKOPT(int, optval, optlen);
            socket->send_buffer_max = value;
            return 0;
        }
        case SO_SNDLOWAT: {
            int value = NET_READ_SOCKOPT(int, optval, optlen);
            socket->send_low_water_mark = value;
            return 0;
        }
        case SO_SNDTIMEO: {
            struct timeval value = NET_READ_SOCKOPT(struct timeval, optval, optlen);
            socket->send_timeout = value;
            return 0;
        }
        case SO_TYPE:
            return -ENOPROTOOPT;
        case SO_BINDTODEVICE: {
            char name[IF_NAMESIZE] = { 0 };
            memcpy(name, optval, MIN(optlen, IF_NAMESIZE) - 1);
            net_for_each_interface(interface) {
                if (strcmp(interface->name, name) == 0) {
                    socket->bound_interface = interface;
                    return 0;
                }
            }
            return -ENODEV;
        }
        default:
            return -ENOPROTOOPT;
    }
}

int net_generic_getsockopt(struct socket *socket, int level, int optname, void *optval, socklen_t *optlen) {
    if (level != SOL_SOCKET) {
        return -ENOPROTOOPT;
    }

    switch (optname) {
        case SO_ACCEPTCONN:
            return NET_WRITE_SOCKOPT(socket->state == LISTENING, int, optval, optlen);
        case SO_BROADCAST:
            return NET_WRITE_SOCKOPT(socket->broadcast, int, optval, optlen);
        case SO_DEBUG:
            return NET_WRITE_SOCKOPT(socket->debug, int, optval, optlen);
        case SO_DONTROUTE:
            return NET_WRITE_SOCKOPT(socket->dont_route, int, optval, optlen);
        case SO_ERROR: {
            int error = socket->error;
            socket->error = 0;
            return NET_WRITE_SOCKOPT(error, int, optval, optlen);
        }
        case SO_KEEPALIVE:
            return NET_WRITE_SOCKOPT(socket->keepalive, int, optval, optlen);
        case SO_LINGER:
            return NET_WRITE_SOCKOPT(socket->linger, struct linger, optval, optlen);
        case SO_OOBINLINE:
            return NET_WRITE_SOCKOPT(socket->oob_inline, int, optval, optlen);
        case SO_RCVBUF:
            return NET_WRITE_SOCKOPT(socket->recv_buffer_max, int, optval, optlen);
        case SO_RCVLOWAT:
            return NET_WRITE_SOCKOPT(socket->recv_low_water_mark, int, optval, optlen);
        case SO_RCVTIMEO:
            return NET_WRITE_SOCKOPT(socket->recv_timeout, struct timeval, optval, optlen);
        case SO_REUSEADDR:
            return NET_WRITE_SOCKOPT(socket->reuse_addr, int, optval, optlen);
        case SO_SNDBUF:
            return NET_WRITE_SOCKOPT(socket->send_buffer_max, int, optval, optlen);
        case SO_SNDLOWAT:
            return NET_WRITE_SOCKOPT(socket->send_low_water_mark, int, optval, optlen);
        case SO_SNDTIMEO:
            return NET_WRITE_SOCKOPT(socket->send_timeout, struct timeval, optval, optlen);
        case SO_TYPE:
            return NET_WRITE_SOCKOPT(socket->type, int, optval, optlen);
        case SO_BINDTODEVICE:
            if (!socket->bound_interface) {
                return -EINVAL;
            }
            *optlen = MIN(strlen(socket->bound_interface->name) + 1, *optlen);
            memcpy(optval, socket->bound_interface->name, *optlen);
            return 0;
        default:
            return -ENOPROTOOPT;
    }
}

int net_get_next_connection(struct socket *socket, struct socket_connection *connection) {
    mutex_lock(&socket->lock);
    for (;;) {
        if (socket->pending[0] != NULL) {
            memcpy(connection, socket->pending[0], sizeof(struct socket_connection));

            free(socket->pending[0]);
            memmove(socket->pending, socket->pending + 1, (socket->pending_length - 1) * sizeof(struct socket_connection *));
            socket->pending[--socket->num_pending] = NULL;

            if (socket->num_pending == 0) {
                fs_detrigger_state(&socket->file_state, POLLIN);
            }

            mutex_unlock(&socket->lock);
            break;
        }

        if (socket->type & SOCK_NONBLOCK) {
            mutex_unlock(&socket->lock);
            return -EAGAIN;
        }

        int ret = net_poll_wait(socket, POLLIN, NULL);
        if (ret) {
            return ret;
        }
    }

    return 0;
}

ssize_t net_send_to_socket(struct socket *to_send, struct socket_data *socket_data) {
    mutex_lock(&to_send->lock);
    insque(socket_data, to_send->data_tail);
    if (!to_send->data_head) {
        to_send->data_head = to_send->data_tail = socket_data;
    } else {
        to_send->data_tail = socket_data;
    }

    fs_trigger_state(&to_send->file_state, POLLIN);

#ifdef SOCKET_DEBUG
    debug_log("Sent message to: [ %p ]\n", to_send);
#endif /* SOCKET_DEBUG */

    ssize_t ret = socket_data->len;
    mutex_unlock(&to_send->lock);
    return ret;
}

void net_socket_set_error(struct socket *socket, int error) {
    socket->error = error;
    fs_trigger_state(&socket->file_state, POLLERR);
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

struct list_node *net_get_protocol_list(void) {
    return &protocol_list;
}

void net_register_protocol(struct socket_protocol *protocol) {
    debug_log("Registering socket protocol: [ %s ]\n", protocol->name);
    list_append(&protocol_list, &protocol->list);
}
