#include <errno.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/hal/processor.h>
#include <kernel/net/destination_cache.h>
#include <kernel/net/inet_socket.h>
#include <kernel/net/interface.h>
#include <kernel/net/socket_syscalls.h>
#include <kernel/net/tcp.h>
#include <kernel/net/tcp_socket.h>
#include <kernel/proc/blockers.h>
#include <kernel/proc/task.h>
#include <kernel/time/clock.h>
#include <kernel/time/timer.h>
#include <kernel/util/hash_map.h>
#include <kernel/util/random.h>

// #define TCP_DEBUG

static int net_tcp_accept(struct socket *socket, struct sockaddr *addr, socklen_t *addrlen, int flags);
static int net_tcp_close(struct socket *socket);
static int net_tcp_connect(struct socket *socket, const struct sockaddr *addr, socklen_t addrlen);
static int net_tcp_destroy(struct socket *socket);
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
    .destroy = net_tcp_destroy,
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

    debug_log("Made TCP socket mapping: [ %d.%d.%d.%d, %u, %d.%d.%d.%d, %u ]\n", mapping->key.source_ip.addr[0],
              mapping->key.source_ip.addr[1], mapping->key.source_ip.addr[2], mapping->key.source_ip.addr[3], mapping->key.source_port,
              mapping->key.dest_ip.addr[0], mapping->key.dest_ip.addr[1], mapping->key.dest_ip.addr[2], mapping->key.dest_ip.addr[3],
              mapping->key.dest_port);
}

void net_remove_tcp_socket_mapping(struct socket *socket) {
    struct tcp_connection_info info = {
        .source_ip = IP_V4_FROM_SOCKADDR(&socket->host_address),
        .source_port = PORT_FROM_SOCKADDR(&socket->host_address),
        .dest_ip = IP_V4_FROM_SOCKADDR(&socket->peer_address),
        .dest_port = PORT_FROM_SOCKADDR(&socket->peer_address),
    };

    struct tcp_socket_mapping *mapping = hash_del_entry(tcp_connection_map, &info, struct tcp_socket_mapping);

    debug_log("Removed TCP socket mapping: [ %d.%d.%d.%d, %u, %d.%d.%d.%d, %u ]\n", mapping->key.source_ip.addr[0],
              mapping->key.source_ip.addr[1], mapping->key.source_ip.addr[2], mapping->key.source_ip.addr[3], mapping->key.source_port,
              mapping->key.dest_ip.addr[0], mapping->key.dest_ip.addr[1], mapping->key.dest_ip.addr[2], mapping->key.dest_ip.addr[3],
              mapping->key.dest_port);

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

struct tcp_control_block *net_allocate_tcp_control_block(struct socket *socket) {
    struct tcp_control_block *tcb = socket->private_data = calloc(1, sizeof(struct tcp_control_block));
    tcb->state = TCP_CLOSED;
    tcb->send_window = 1;
    tcb->send_unacknowledged = tcb->send_next = get_random_bytes() & 0xFFFFF;       // Initial sequence number
    tcb->recv_window = 8192;                                                        // Hard coded value
    tcb->recv_mss = 1024 - sizeof(struct ip_v4_packet) - sizeof(struct tcp_packet); // Default MSS minus headers
    tcb->rto = (struct timespec) { .tv_sec = 1, .tv_nsec = 0 };                     // RTO starts at 1 second.
    tcb->first_rtt_sample = true;
    init_ring_buffer(&tcb->send_buffer, 8192);
    init_ring_buffer(&tcb->recv_buffer, tcb->recv_window);
    return tcb;
}

void net_free_tcp_control_block(struct socket *socket) {
    struct tcp_control_block *tcb = socket->private_data;
    socket->private_data = NULL;

    net_remove_tcp_socket_mapping(socket);
    if (tcb->time_wait_timer) {
        time_cancel_kernel_callback(tcb->time_wait_timer);
    }
    if (tcb->rto_timer) {
        time_cancel_kernel_callback(tcb->rto_timer);
    }
    if (tcb->send_ack_timer) {
        time_cancel_kernel_callback(tcb->send_ack_timer);
    }
    if (tcb->destination) {
        net_drop_destination_cache_entry(tcb->destination);
    }
    kill_ring_buffer(&tcb->send_buffer);
    kill_ring_buffer(&tcb->recv_buffer);
    free(tcb);
}

static void tcp_do_retransmit(struct timer *timer, void *_socket) {
    struct socket *socket = _socket;
    struct tcp_control_block *tcb = socket->private_data;

    // Retransmit the earliest segment not acknowledged by the sender.
    size_t max_data_to_retransmit = tcb->send_mss - sizeof(struct tcp_packet) - sizeof(struct ip_v4_packet);
    size_t data_to_retransmit = MIN(tcb->send_next - tcb->send_unacknowledged, max_data_to_retransmit);
    net_send_tcp_from_socket(socket, tcb->send_unacknowledged, tcb->send_unacknowledged + data_to_retransmit, false, true);

    // Exponential back off by doubling the next timeout.
    tcb->rto = time_add(tcb->rto, tcb->rto);
    __time_reset_kernel_callback(timer, &tcb->rto);

    if (tcb->pending_syn) {
        // Since the initial SYN segment was lost, RTO must be initialized to 3 seconds later.
        tcb->reset_rto_once_established = true;
    }

    // The round trip time can't be measured now that a segment has been retransmitted.
    tcb->time_first_sent_valid = false;
}

static void tcp_setup_retransmission_timer(struct socket *socket) {
    struct tcp_control_block *tcb = socket->private_data;
    if (tcb->send_unacknowledged == tcb->send_next) {
        return;
    }

    if (!tcb->rto_timer) {
        tcb->rto_timer = time_register_kernel_callback(&tcb->rto, tcp_do_retransmit, socket);
    } else {
        time_reset_kernel_callback(tcb->rto_timer, &tcb->rto);
    }
}

bool tcp_update_recv_window(struct socket *socket) {
    struct tcp_control_block *tcb = socket->private_data;
    uint16_t window_max = ring_buffer_max(&tcb->recv_buffer);
    uint16_t actual_window_size = ring_buffer_space(&tcb->recv_buffer);

    uint16_t new_size;
    // Only update the window size when the halfway mark is reached to avoid SWS.
    if (actual_window_size == 0) {
        new_size = 0;
    } else if (actual_window_size < window_max / 2) {
        new_size = window_max / 2;
    } else {
        new_size = window_max;
    }

    bool changed = tcb->recv_window != new_size;
    tcb->recv_window = new_size;
    return changed;
}

int tcp_send_segments(struct socket *socket) {
    struct tcp_control_block *tcb = socket->private_data;
    size_t segment_size = tcb->send_mss - sizeof(struct ip_v4_packet) - sizeof(struct tcp_packet);
    bool push_data = true; // Assume all data should have the PSH bit set.

    int ret = 0;
    while (ret == 0) {
        ssize_t usable_window = (ssize_t) tcb->send_unacknowledged + (ssize_t) tcb->send_window - (ssize_t) tcb->send_next;
        if (usable_window < 0) {
            break;
        }

        uint32_t data_available = ring_buffer_size(&tcb->send_buffer) + tcb->pending_syn + tcb->pending_fin;
        uint32_t data_queued = data_available - (tcb->send_next - tcb->send_unacknowledged);

        uint32_t data_to_send = 0;
        if (MIN(usable_window, data_queued) >= (ssize_t) segment_size) {
            data_to_send = MIN(usable_window, data_queued);
        } else if ((socket->tcp_nodelay || tcb->send_next == tcb->send_unacknowledged) && push_data && data_queued <= usable_window) {
            data_to_send = data_queued;
        } else if ((socket->tcp_nodelay || tcb->send_next == tcb->send_unacknowledged) &&
                   MIN(usable_window, data_queued) >= tcb->send_window_max / 2) {
            data_to_send = tcb->send_window_max / 2;
        } else if (push_data && data_queued != 0) {
            // FIXME: Set up a timer to send the data in ~0.5s
            break;
        }

        if (data_to_send == 0) {
            break;
        }

        tcb->send_next += data_to_send;
#ifdef TCP_DEBUG
        debug_log("Sending TCP segment: [ %u, %u, %u, %u ]\n", tcb->send_unacknowledged, tcb->send_next - data_to_send, tcb->send_next,
                  usable_window);
#endif /* TCP_DEBUG */
        ret = net_send_tcp_from_socket(socket, tcb->send_next - data_to_send, tcb->send_next, false, false);
        tcp_setup_retransmission_timer(socket);
    }

    return ret;
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

    struct tcp_control_block *tcb = new_socket->private_data = connection.connect_tcb;
    create_tcp_socket_mapping(new_socket);

    struct ip_v4_address dest_ip = IP_V4_FROM_SOCKADDR(&new_socket->peer_address);
    tcb->interface = net_get_interface_for_ip(dest_ip);
    tcb->destination = net_lookup_destination(tcb->interface, dest_ip);

    // Send a SYN-ACK
    tcb->pending_syn = true;
    tcp_send_segments(new_socket);

    return fd;
}

static int net_tcp_close(struct socket *socket) {
    int ret = 0;
    mutex_lock(&socket->lock);
    struct tcp_control_block *tcb = socket->private_data;
    if (!tcb) {
        mutex_unlock(&socket->lock);
        return 0;
    }

    // If there is data the application hasn't read, a RST segment should be sent to show that data has been lost.
    if (!ring_buffer_empty(&tcb->recv_buffer)) {
        net_send_tcp_from_socket(socket, tcb->send_next, tcb->send_next, true, false);
        net_free_tcp_control_block(socket);
        mutex_unlock(&socket->lock);
        return -ECONNRESET;
    }

    switch (tcb->state) {
        case TCP_LITSEN:
            net_free_tcp_control_block(socket);
            break;
        case TCP_SYN_SENT:
            net_free_tcp_control_block(socket);
            break;
        case TCP_SYN_RECIEVED:
            // No data has been sent yet.
            if (tcb->send_unacknowledged == tcb->send_next - 1) {
                tcb->pending_fin = true;
                tcb->state = TCP_FIN_WAIT_1;
                ret = tcp_send_segments(socket);
            } else {
                tcb->pending_fin = true;
            }
            break;
        case TCP_ESTABLISHED:
            tcb->pending_fin = true;
            tcb->state = TCP_FIN_WAIT_1;
            ret = tcp_send_segments(socket);
            break;
        case TCP_FIN_WAIT_1:
        case TCP_FIN_WAIT_2:
            break;
        case TCP_CLOSE_WAIT:
            tcb->pending_fin = true;
            tcb->state = TCP_LAST_ACK;
            ret = tcp_send_segments(socket);
            break;
        case TCP_CLOSING:
        case TCP_LAST_ACK:
        case TCP_TIME_WAIT:
            ret = ENOTCONN;
            break;
        default:
            assert(false);
    }

    mutex_unlock(&socket->lock);
    return ret;
}

static int net_tcp_connect(struct socket *socket, const struct sockaddr *addr, socklen_t addrlen) {
    if (addrlen < sizeof(struct sockaddr_in)) {
        return -EINVAL;
    }

    if (socket->state != BOUND) {
        struct sockaddr_in to_bind = { .sin_family = AF_INET, .sin_port = 0, .sin_addr = { .s_addr = INADDR_ANY }, .sin_zero = { 0 } };
        int ret = tcp_ops.bind(socket, (struct sockaddr *) &to_bind, sizeof(struct sockaddr_in));
        if (ret < 0) {
            return ret;
        }
    }

    net_set_peer_address(socket, addr, addrlen);

    struct tcp_control_block *tcb = net_allocate_tcp_control_block(socket);
    tcb->state = TCP_SYN_SENT;

    create_tcp_socket_mapping(socket);

    struct ip_v4_address dest_ip = IP_V4_FROM_SOCKADDR(&socket->peer_address);
    tcb->interface = net_get_interface_for_ip(dest_ip);
    tcb->destination = net_lookup_destination(tcb->interface, dest_ip);

    tcb->pending_syn = true;
    int ret = tcp_send_segments(socket);
    if (ret) {
        net_free_tcp_control_block(socket);
        return ret;
    }

    return 0;
}

static int net_tcp_destroy(struct socket *socket) {
    struct tcp_control_block *tcb = socket->private_data;
    if (tcb) {
        net_free_tcp_control_block(socket);
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
        struct sockaddr_in addr = { .sin_family = AF_INET, .sin_port = 0, .sin_addr = { .s_addr = INADDR_ANY }, .sin_zero = { 0 } };
        int ret = tcp_ops.bind(socket, (struct sockaddr *) &addr, sizeof(struct sockaddr_in));
        if (ret) {
            return ret;
        }
    }

    struct tcp_control_block *tcb = net_allocate_tcp_control_block(socket);
    tcb->state = TCP_LITSEN;
    tcb->is_passive = true;

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
            socket->readable = false;
            if (tcb->state == TCP_CLOSE_WAIT) {
                break;
            }

            if (socket->error != 0) {
                // Only clear socket->error if the error will actually be returned.
                error = -socket->error;
                if (buffer_index != 0) {
                    socket->error = 0;
                }
                goto done;
            }

            mutex_unlock(&socket->lock);
            int ret = net_block_until_socket_is_readable(socket, start_time);
            if (ret) {
                return ret;
            }
            mutex_lock(&socket->lock);
            continue;
        }

        size_t amount_to_read = MIN(amount_readable, len - buffer_index);
        ring_buffer_user_read(&tcb->recv_buffer, buffer + buffer_index, amount_to_read);
        buffer_index += amount_to_read;
        tcp_update_recv_window(socket);
        socket->readable = !ring_buffer_empty(&tcb->recv_buffer);

        // Instead of always breaking here, it is probably more correct to only do so if the PSH flag was set.
        break;
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
                return ret;
            }
            mutex_lock(&socket->lock);
            continue;
        }

        size_t amount_to_write = MIN(space_available, len - buffer_index);
        ring_buffer_user_write(&tcb->send_buffer, buffer + buffer_index, amount_to_write);
        buffer_index += amount_to_write;
        socket->writable = !ring_buffer_full(&tcb->send_buffer);

        // Only send the data once the ESTABLISHED state is reached.
        if (tcb->state != TCP_SYN_RECIEVED && tcb->state != TCP_SYN_SENT) {
            error = tcp_send_segments(socket);
            if (error) {
                break;
            }
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
