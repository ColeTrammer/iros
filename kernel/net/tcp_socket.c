#include <errno.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/hal/processor.h>
#include <kernel/net/inet_socket.h>
#include <kernel/net/socket_syscalls.h>
#include <kernel/net/tcp.h>
#include <kernel/net/tcp_socket.h>
#include <kernel/proc/blockers.h>
#include <kernel/proc/task.h>
#include <kernel/time/clock.h>
#include <kernel/time/timer.h>
#include <kernel/util/hash_map.h>

static int net_tcp_accept(struct socket *socket, struct sockaddr *addr, socklen_t *addrlen, int flags);
static int net_tcp_close(struct socket *socket);
static int net_tcp_connect(struct socket *socket, const struct sockaddr *addr, socklen_t addrlen);
static int net_tcp_getsockopt(struct socket *socket, int level, int optname, void *optval, socklen_t *optlen);
static int net_tcp_setsockopt(struct socket *socket, int level, int optname, const void *optval, socklen_t optlen);
static int net_tcp_listen(struct socket *socket, int backlog);
static int net_tcp_socket(int domain, int type, int protocol);
static ssize_t net_tcp_recvfrom(struct socket *socket, void *buf, size_t len, int flags, struct sockaddr *addr, socklen_t *addrlen);
static ssize_t net_tcp_sendto(struct socket *socket, const void *buf, size_t len, int flags, const struct sockaddr *addr,
                              socklen_t addrlen);

static struct socket_ops tcp_ops = {
    .accept = net_tcp_accept,
    .bind = net_inet_bind,
    .close = net_tcp_close,
    .connect = net_tcp_connect,
    .getpeername = net_inet_getpeername,
    .getsockname = net_inet_getsockname,
    .getsockopt = net_tcp_getsockopt,
    .setsockopt = net_tcp_setsockopt,
    .listen = net_tcp_listen,
    .sendto = net_tcp_sendto,
    .recvfrom = net_tcp_recvfrom,
};

static struct socket_protocol tcp_protocol = {
    .domain = AF_INET,
    .type = SOCK_STREAM,
    .protocol = IPPROTO_TCP,
    .is_default_protocol = true,
    .name = "IPv4 TCP",
    .create_socket = net_tcp_socket,
};

static struct hash_map *tcp_connection_map;

static unsigned int tcp_connection_info_hash(void *i, int num_buckets) {
    struct tcp_connection_info *a = i;
    return (a->source_ip.addr[0] + a->source_ip.addr[1] + a->source_ip.addr[2] + a->source_ip.addr[3] + a->source_port +
            a->dest_ip.addr[0] + a->dest_ip.addr[1] + a->dest_ip.addr[2] + a->dest_ip.addr[3] + a->dest_port) %
           num_buckets;
}

static int tcp_connection_info_equals(void *i1, void *i2) {
    return memcmp(i1, i2, sizeof(struct tcp_connection_info)) == 0;
}

static void *tcp_connection_info_key(struct hash_entry *m) {
    return &hash_table_entry(m, struct tcp_socket_mapping)->key;
}

static void create_tcp_socket_mapping(struct socket *socket) {
    struct tcp_socket_mapping *mapping = calloc(1, sizeof(struct tcp_socket_mapping));
    // Even when the peer address is not yet known (in the case of a listening socket), the peer address
    // will still be initialzed correctly (as all zeroes).
    mapping->key = (struct tcp_connection_info) {
        .source_ip = IP_V4_FROM_SOCKADDR(&socket->host_address),
        .source_port = PORT_FROM_SOCKADDR(&socket->host_address),
        .dest_ip = IP_V4_FROM_SOCKADDR(&socket->peer_address),
        .dest_port = PORT_FROM_SOCKADDR(&socket->peer_address),
    };
    mapping->socket = net_bump_socket(socket);

    hash_put(tcp_connection_map, &mapping->hash);
}

void net_remove_tcp_socket_mapping(struct socket *socket) {
    struct tcp_connection_info info = {
        .source_ip = IP_V4_FROM_SOCKADDR(&socket->host_address),
        .source_port = PORT_FROM_SOCKADDR(&socket->host_address),
        .dest_ip = IP_V4_FROM_SOCKADDR(&socket->peer_address),
        .dest_port = PORT_FROM_SOCKADDR(&socket->peer_address),
    };

    struct tcp_socket_mapping *mapping = hash_del_entry(tcp_connection_map, &info, struct tcp_socket_mapping);
    net_drop_socket(mapping->socket);
    free(mapping);
}

struct socket *net_get_tcp_socket_by_connection_info(struct tcp_connection_info *info) {
    struct tcp_socket_mapping *mapping = hash_get_entry(tcp_connection_map, info, struct tcp_socket_mapping);
    if (mapping == NULL) {
        return NULL;
    }

    return mapping->socket;
}

static struct tcp_control_block *tcp_allocate_control_block(struct socket *socket) {
    struct tcp_control_block *tcb = socket->private_data = calloc(1, sizeof(struct tcp_control_block));
    tcb->state = TCP_CLOSED;
    tcb->send_unacknowledged = tcb->send_next = 10000;                           // Initial sequence number
    tcb->recv_window = 8192;                                                     // Hard coded value
    tcb->segment_size = 1500;                                                    // MTU for ethernet
    tcb->retransmission_delay = (struct timespec) { .tv_sec = 1, .tv_nsec = 0 }; // Start retransmit timer out at 1 second.
    init_ring_buffer(&tcb->send_buffer, 8192);
    init_ring_buffer(&tcb->recv_buffer, tcb->recv_window);
    return tcb;
}

void net_free_tcp_control_block(struct socket *socket) {
    struct tcp_control_block *tcb = socket->private_data;
    net_remove_tcp_socket_mapping(socket);
    if (tcb->retransmission_timer) {
        time_delete_timer(tcb->retransmission_timer);
    }
    kill_ring_buffer(&tcb->send_buffer);
    kill_ring_buffer(&tcb->recv_buffer);
    free(socket->private_data);
    socket->private_data = NULL;
}

static void tcp_do_retransmit(struct timer *timer, void *_socket) {
    struct socket *socket = _socket;
    struct tcp_control_block *tcb = socket->private_data;

    net_send_tcp_from_socket(socket);

    // Exponential back off by doubling the next timeout.
    tcb->retransmission_delay = time_add(tcb->retransmission_delay, tcb->retransmission_delay);
    __time_reset_kernel_callback(timer, &tcb->retransmission_delay);
}

static void tcp_setup_retransmission_timer(struct socket *socket) {
    struct tcp_control_block *tcb = socket->private_data;
    if (tcb->send_unacknowledged == tcb->send_next) {
        return;
    }

    assert(!tcb->retransmission_timer);
    tcb->retransmission_timer = time_register_kernel_callback(&tcb->retransmission_delay, tcp_do_retransmit, socket);
}

static int tcp_send_segment(struct socket *socket, union tcp_flags flags) {
    struct tcp_control_block *tcb = socket->private_data;
    tcb->pending_syn = flags.bits.syn;
    tcb->pending_fin = flags.bits.fin;

    size_t data_to_send = MIN(ring_buffer_size(&tcb->send_buffer), tcb->segment_size);
    tcb->send_next += flags.bits.syn + flags.bits.fin + data_to_send;
    int ret = net_send_tcp_from_socket(socket);
    tcp_setup_retransmission_timer(socket);
    return ret;
}

static int tcp_maybe_send_segment(struct socket *socket) {
    struct tcp_control_block *tcb = socket->private_data;
    if (tcb->retransmission_timer) {
        return 0;
    }

    return tcp_send_segment(socket, (union tcp_flags) { .bits = { .ack = 1 } });
}

static int net_tcp_accept(struct socket *socket, struct sockaddr *addr, socklen_t *addrlen, int flags) {
    struct socket_connection connection;
    int ret = net_get_next_connection(socket, &connection);
    if (ret != 0) {
        return ret;
    }

    if (addr) {
        net_copy_sockaddr_to_user(&connection.addr.in, sizeof(struct sockaddr_in), addr, addrlen);
    }

    debug_log("Creating connection: [ %p ]\n", socket);

    int fd;
    struct socket *new_socket = net_create_socket_fd(AF_INET, (SOCK_STREAM & SOCK_TYPE_MASK) | flags, IPPROTO_TCP, &tcp_ops, &fd, NULL);
    if (new_socket == NULL) {
        return fd;
    }

    net_set_peer_address(new_socket, &connection.addr.in, sizeof(connection.addr.in));
    net_set_host_address(new_socket, &socket->host_address, sizeof(socket->host_address));

    return fd;
}

static int net_tcp_close(struct socket *socket) {
    struct tcp_control_block *tcb = socket->private_data;
    if (tcb) {
        net_free_tcp_control_block(socket);
    }

    return net_inet_close(socket);
}

static int net_tcp_connect(struct socket *socket, const struct sockaddr *addr, socklen_t addrlen) {
    if (addrlen < sizeof(struct sockaddr_in)) {
        return -EINVAL;
    }

    if (socket->state != BOUND) {
        struct sockaddr_in to_bind = { AF_INET, 0, { INADDR_NONE }, { 0 } };
        int ret = tcp_ops.bind(socket, (struct sockaddr *) &to_bind, sizeof(struct sockaddr_in));
        if (ret < 0) {
            return ret;
        }
    }

    net_set_peer_address(socket, addr, addrlen);

    struct tcp_control_block *tcb = tcp_allocate_control_block(socket);
    tcb->state = TCP_SYN_SENT;

    create_tcp_socket_mapping(socket);

    int ret = tcp_send_segment(socket, (union tcp_flags) { .bits = { .syn = 1 } });
    if (ret) {
        net_free_tcp_control_block(socket);
        return ret;
    }

    return 0;
}

static int net_tcp_getsockopt(struct socket *socket, int level, int optname, void *optval, socklen_t *optlen) {
    if (level != IPPROTO_TCP) {
        return net_generic_getsockopt(socket, level, optname, optval, optlen);
    }

    switch (optname) {
        case TCP_NODELAY:
            return NET_WRITE_SOCKOPT(socket->tcp_nodelay, int, optval, optlen);
        default:
            return -ENOPROTOOPT;
    }
}

static int net_tcp_setsockopt(struct socket *socket, int level, int optname, const void *optval, socklen_t optlen) {
    if (level != IPPROTO_TCP) {
        return net_generic_setsockopt(socket, level, optname, optval, optlen);
    }

    switch (optname) {
        case TCP_NODELAY: {
            int value = NET_READ_SOCKOPT(int, optval, optlen);
            socket->tcp_nodelay = !!value;
            return 0;
        }
        default:
            return -ENOPROTOOPT;
    }
}

static int net_tcp_listen(struct socket *socket, int backlog) {
    if (socket->state != BOUND) {
        struct sockaddr_in addr = { .sin_family = AF_INET, .sin_port = 0, .sin_addr = { .s_addr = INADDR_NONE }, .sin_zero = { 0 } };
        int ret = tcp_ops.bind(socket, (struct sockaddr *) &addr, sizeof(struct sockaddr_in));
        if (ret) {
            return ret;
        }
    }

    struct tcp_control_block *tcb = tcp_allocate_control_block(socket);
    tcb->state = TCP_LITSEN;

    create_tcp_socket_mapping(socket);

    return net_generic_listen(socket, backlog);
}

static int net_tcp_socket(int domain, int type, int protocol) {
    int fd;
    net_create_socket_fd(domain, type, protocol, &tcp_ops, &fd, NULL);
    return fd;
}

static ssize_t net_tcp_recvfrom(struct socket *socket, void *buffer, size_t len, int flags, struct sockaddr *addr, socklen_t *addrlen) {
    (void) flags;
    (void) addr;
    (void) addrlen;

    mutex_lock(&socket->lock);
    if (addr) {
        net_copy_sockaddr_to_user(&socket->peer_address, sizeof(struct sockaddr_in), addr, addrlen);
    }

    size_t buffer_index = 0;
    int error = 0;

    struct timespec start_time = time_read_clock(CLOCK_MONOTONIC);
    while (buffer_index < len) {
        struct tcp_control_block *tcb = socket->private_data;
        if (!tcb) {
            error = socket->error;
            break;
        }

        int ret = tcp_maybe_send_segment(socket);
        if (ret) {
            error = ret;
            goto done;
        }

        switch (tcb->state) {
            case TCP_CLOSING:
            case TCP_LAST_ACK:
            case TCP_TIME_WAIT:
                error = -ENOTCONN;
                goto done;
            default:
                break;
        }

        size_t amount_readable = ring_buffer_size(&tcb->recv_buffer);
        if (!amount_readable) {
            if (tcb->state == TCP_CLOSE_WAIT) {
                break;
            }

            socket->readable = false;
            mutex_unlock(&socket->lock);
            int ret = net_block_until_socket_is_readable(socket, start_time);
            if (ret) {
                error = ret;
                break;
            }
            mutex_lock(&socket->lock);
            continue;
        }

        size_t amount_to_read = MIN(amount_readable, len - buffer_index);
        ring_buffer_user_read(buffer + buffer_index, &tcb->recv_buffer, amount_to_read);
        buffer_index += amount_to_read;
        tcb->recv_window += amount_to_read;
        socket->readable = !ring_buffer_empty(&tcb->recv_buffer);

        error = tcp_maybe_send_segment(socket);
        if (error) {
            break;
        }
    }
done:
    mutex_unlock(&socket->lock);

    return buffer_index ? (ssize_t) buffer_index : error;
}

static ssize_t net_tcp_sendto(struct socket *socket, const void *buffer, size_t len, int flags, const struct sockaddr *addr,
                              socklen_t addrlen) {
    (void) flags;

    if (!!addr || !!addrlen) {
        return -EINVAL;
    }

    size_t buffer_index = 0;
    int error = 0;

    struct timespec start_time = time_read_clock(CLOCK_MONOTONIC);
    mutex_lock(&socket->lock);
    while (buffer_index < len) {
        struct tcp_control_block *tcb = socket->private_data;
        if (!tcb) {
            error = socket->error;
            break;
        }

        switch (tcb->state) {
            case TCP_FIN_WAIT_1:
            case TCP_FIN_WAIT_2:
            case TCP_CLOSING:
            case TCP_LAST_ACK:
            case TCP_TIME_WAIT:
                error = -ENOTCONN;
                goto done;
            default:
                break;
        }

        size_t space_available = ring_buffer_space(&tcb->send_buffer);
        if (!space_available) {
            mutex_unlock(&socket->lock);
            int ret = net_block_until_socket_is_writable(socket, start_time);
            if (ret) {
                error = ret;
                break;
            }
            mutex_lock(&socket->lock);
            continue;
        }

        size_t amount_to_write = MIN(space_available, len - buffer_index);
        ring_buffer_user_write(&tcb->send_buffer, buffer + buffer_index, amount_to_write);
        buffer_index += amount_to_write;
        socket->writable = !ring_buffer_full(&tcb->send_buffer);

        error = tcp_maybe_send_segment(socket);
        if (error) {
            break;
        }
    }
done:
    mutex_unlock(&socket->lock);

    return buffer_index ? (ssize_t) buffer_index : error;
}

void init_tcp_sockets(void) {
    tcp_connection_map = hash_create_hash_map(tcp_connection_info_hash, tcp_connection_info_equals, tcp_connection_info_key);
    assert(tcp_connection_map);

    net_register_protocol(&tcp_protocol);
}
