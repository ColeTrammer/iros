#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/hal/output.h>
#include <kernel/net/destination_cache.h>
#include <kernel/net/ethernet.h>
#include <kernel/net/inet_socket.h>
#include <kernel/net/interface.h>
#include <kernel/net/ip.h>
#include <kernel/net/network_task.h>
#include <kernel/net/packet.h>
#include <kernel/net/tcp.h>
#include <kernel/net/tcp_socket.h>
#include <kernel/time/clock.h>
#include <kernel/time/timer.h>
#include <kernel/util/macros.h>

// #define TCP_DEBUG

// 2 * MSL (which is 2 * 2 minutes)
static struct timespec time_wait_timeout = { .tv_sec = 240, .tv_nsec = 0 };

// Hardcoded to 300 ms, should probably be based on round trip time.
static struct timespec ack_timeout = { .tv_sec = 0, .tv_nsec = 300 * 1000000 };

int net_send_tcp_from_socket(struct socket *socket, uint32_t sequence_start, uint32_t sequence_end, bool is_retransmission) {
    uint16_t source_port = PORT_FROM_SOCKADDR(&socket->host_address);
    struct ip_v4_address dest_ip = IP_V4_FROM_SOCKADDR(&socket->peer_address);
    uint16_t dest_port = PORT_FROM_SOCKADDR(&socket->peer_address);

    struct tcp_control_block *tcb = socket->private_data;

    // No need to send an ACK later, this segment counts as an acknowledgement.
    if (tcb->send_ack_timer) {
        time_cancel_kernel_callback(tcb->send_ack_timer);
        tcb->send_ack_timer = NULL;
    }

    // Only send a SYN if this is the first pending segment, only send a FIN if this is the last pending segment.
    bool send_syn = tcb->pending_syn && sequence_start == tcb->send_unacknowledged;
    bool send_fin = tcb->pending_fin && sequence_end == tcb->send_next;
    size_t data_to_send = sequence_end - sequence_start - send_syn - send_fin;

    struct tcp_packet_options opts = {
        .source_port = source_port,
        .dest_port = dest_port,
        .sequence_number = sequence_start,
        .ack_number = tcb->recv_next,
        .window = tcb->recv_window,
        .mss = tcb->recv_mss,
        .tcp_flags = {
            .ack = tcb->state != TCP_SYN_SENT, // The only segment that doesn't have ACK set is the initial SYN.
            .psh = !send_syn && !send_fin,     // Pretend all data segments are pushed for now.
            .syn = send_syn,
            .fin = send_fin,
        },
        .data_offset = sequence_start - tcb->send_unacknowledged,
        .data_length = data_to_send,
        .data_rb = &tcb->send_buffer,
        .socket = socket,
        .interface = tcb->interface,
        .destination = tcb->destination,
    };

    struct timespec *send_time_ptr = NULL;

    // The round trip time can only be sampled if its not already being sampled, this packet requires acknowledgement,
    // and the acknowledgement could not have already been sent.
    if (!tcb->time_first_sent_valid && !is_retransmission && sequence_start != sequence_end && sequence_end == tcb->send_next) {
        send_time_ptr = &tcb->time_first_sent;
        tcb->time_first_sent_sequence_number = sequence_start;
        tcb->time_first_sent_valid = true;
    }

    return net_send_tcp(dest_ip, &opts, send_time_ptr);
}

int net_send_tcp(struct ip_v4_address dest, struct tcp_packet_options *opts, struct timespec *send_time_ptr) {
    struct network_interface *interface = opts->interface ? opts->interface : net_get_interface_for_ip(dest);
    if (interface->config_context.state != INITIALIZED) {
        debug_log("Can't send TCP packet; interface uninitialized: [ %s ]\n", interface->name);
        return -ENETDOWN;
    }

    struct destination_cache_entry *destination = opts->destination ? opts->destination : net_lookup_destination(interface, dest);
    size_t tcp_length = sizeof(struct tcp_packet) + opts->tcp_flags.syn * sizeof(struct tcp_option_mss) + opts->data_length;

    struct packet *packet = net_create_packet(interface, opts->socket, destination, tcp_length);
    packet->header_count = interface->link_layer_overhead + 2;

    struct packet_header *tcp_header =
        net_init_packet_header(packet, interface->link_layer_overhead + 1, PH_TCP, packet->inline_data, tcp_length);
    struct tcp_packet *tcp_packet = tcp_header->raw_header;
    net_init_tcp_packet(tcp_packet, opts);

    struct ip_v4_pseudo_header header = { interface->address, dest, 0, IP_V4_PROTOCOL_TCP, htons(tcp_length) };
    tcp_packet->check_sum = ntohs(in_compute_checksum_with_start(tcp_packet, tcp_length, in_compute_checksum(&header, sizeof(header))));

    int ret = interface->ops->send_ip_v4(interface, destination, packet);
    if (send_time_ptr) {
        *send_time_ptr = time_read_clock(CLOCK_MONOTONIC);
    }

    if (!opts->destination) {
        net_drop_destination_cache_entry(destination);
    }
    return ret;
}

static size_t tcp_segment_length(const struct ip_v4_packet *ip_packet, const struct tcp_packet *packet) {
    return ntohs(ip_packet->length) - sizeof(struct ip_v4_packet) - sizeof(uint32_t) * packet->data_offset + packet->flags.syn +
           packet->flags.fin;
}

static const void *tcp_data_start(const struct tcp_packet *packet) {
    return (const void *) packet + sizeof(uint32_t) * packet->data_offset;
}

static uint16_t tcp_mss(const struct tcp_packet *packet) {
    size_t offset = 0;
    size_t end = sizeof(uint32_t) * packet->data_offset - sizeof(struct tcp_packet);
    const uint8_t *raw_options = (uint8_t *) (packet + 1);
    while (offset < end) {
        switch (raw_options[offset]) {
            case TCP_OPTION_END:
                break;
            case TCP_OPTION_PAD:
                offset++;
                continue;
            case TCP_OPTION_MSS: {
                const struct tcp_option_mss *mss_option = (const struct tcp_option_mss *) raw_options;
                return ntohs(mss_option->mss);
            }
            default: {
                const struct tcp_option *option = (const struct tcp_option *) raw_options;
                debug_log("Unknown TCP option type: [ %u ]\n", option->type);
                if (option->length < 2) {
                    break;
                }
                offset += option->length;
                continue;
            }
        }
        break;
    }

    return TCP_DEFAULT_MSS;
}

static void tcp_send_reset(const struct ip_v4_packet *ip_packet, const struct tcp_packet *packet) {
    if (packet->flags.rst) {
        return;
    }

    struct tcp_packet_options opts = {
        .source_port = htons(packet->dest_port),
        .dest_port = htons(packet->source_port),
        .sequence_number = 0,
        .ack_number = 0,
        .window = 0,
        .tcp_flags = { 
            .rst = true, 
        },
        .data_offset = 0,
        .data_length = 0,
        .data_rb = NULL,
        .socket = NULL,
        .interface = NULL,
        .destination = NULL,
    };

    if (packet->flags.ack) {
        opts.sequence_number = htonl(packet->ack_number);
    } else {
        opts.ack_number = htonl(packet->sequence_number) + tcp_segment_length(ip_packet, packet);
        opts.tcp_flags.ack = true;
    }
    net_send_tcp(ip_packet->source, &opts, NULL);
}

static void tcp_send_empty_ack(struct socket *socket, const struct ip_v4_packet *ip_packet, const struct tcp_packet *packet) {
    if (packet->flags.rst) {
        return;
    }

    struct tcp_control_block *tcb = socket->private_data;
    if (tcb->send_ack_timer) {
        time_cancel_kernel_callback(tcb->send_ack_timer);
        tcb->send_ack_timer = NULL;
    }

    struct tcp_packet_options opts = {
        .source_port = htons(packet->dest_port),
        .dest_port = htons(packet->source_port),
        .sequence_number = tcb->send_next,
        .ack_number = tcb->recv_next,
        .window = tcb->recv_window,
        .tcp_flags = {
            .ack = 1,
        },
        .data_offset = 0,
        .data_length = 0,
        .data_rb = NULL,
        .socket = NULL,
        .interface = tcb->interface,
        .destination = tcb->destination,
    };

    net_send_tcp(ip_packet->source, &opts, NULL);
}

static void tcp_time_wait_expiration(struct timer *timer __attribute__((unused)), void *socket) {
    net_free_tcp_control_block(socket);
}

static void tcp_on_ack_timeout(struct timer *timer, void *_socket) {
    struct socket *socket = _socket;
    struct tcp_control_block *tcb = socket->private_data;

    uint16_t source_port = PORT_FROM_SOCKADDR(&socket->host_address);
    struct ip_v4_address dest_ip = IP_V4_FROM_SOCKADDR(&socket->peer_address);
    uint16_t dest_port = PORT_FROM_SOCKADDR(&socket->peer_address);

    struct tcp_packet_options opts = {
        .source_port = source_port,
        .dest_port = dest_port,
        .sequence_number = tcb->send_next,
        .ack_number = tcb->recv_next,
        .window = tcb->recv_window,
        .tcp_flags = {
            .ack = 1,
        },
        .data_offset = 0,
        .data_length = 0,
        .data_rb = NULL,
        .socket = socket,
        .interface = tcb->interface,
        .destination = tcb->destination,
    };

    time_delete_timer(timer);
    tcb->send_ack_timer = NULL;

    net_send_tcp(dest_ip, &opts, NULL);
}

static bool tcp_segment_acceptable(struct socket *socket, const struct ip_v4_packet *ip_packet, const struct tcp_packet *packet) {
    struct tcp_control_block *tcb = socket->private_data;
    size_t recv_window = tcb->recv_window;
    size_t recv_next = tcb->recv_next;
    size_t segment_length = tcp_segment_length(ip_packet, packet);
    uint32_t seq_num = htonl(packet->sequence_number);

    if (segment_length == 0 && recv_window == 0) {
        return seq_num == recv_next;
    }
    if (segment_length == 0 && recv_window > 0) {
        return recv_next <= seq_num && seq_num < recv_next + recv_window;
    }
    if (segment_length > 0 && recv_window == 0) {
        return false;
    }
    return (recv_next <= seq_num && seq_num < recv_next + recv_window) ||
           (recv_next <= seq_num + segment_length && seq_num + segment_length < recv_next + recv_window);
}

static void tcp_update_send_window(struct socket *socket, const struct tcp_packet *packet) {
    struct tcp_control_block *tcb = socket->private_data;
    tcb->send_window = htons(packet->window_size);
    tcb->send_window_max = MAX(tcb->send_window_max, tcb->send_window);
    tcb->send_wl1 = htonl(packet->sequence_number);
    tcb->send_wl2 = htonl(packet->ack_number);
}

static void tcp_enter_established_state(struct socket *socket, const struct tcp_packet *packet) {
    struct tcp_control_block *tcb = socket->private_data;
    tcb->state = TCP_ESTABLISHED;
    if (tcb->reset_rto_once_established && tcb->rto.tv_sec < 3) {
        tcb->rto = (struct timespec) { .tv_sec = 3, .tv_nsec = 0 };
    }
    tcp_update_send_window(socket, packet);
    tcp_send_segments(socket);
}

static void tcp_sample_round_trip_time(struct socket *socket) {
    struct tcp_control_block *tcb = socket->private_data;
    struct timespec measured_rtt = time_sub(time_read_clock(CLOCK_MONOTONIC), tcb->time_first_sent);
    time_t rtt = measured_rtt.tv_sec * 1000 + measured_rtt.tv_nsec / 1000000;
    if (tcb->first_rtt_sample) {
        tcb->smoothed_rtt = rtt;
        tcb->rtt_variation = rtt / 2;
        tcb->first_rtt_sample = false;
    } else {
        tcb->rtt_variation = tcb->rtt_variation * 3 / 4 + labs(tcb->smoothed_rtt - rtt) / 4;
        tcb->smoothed_rtt = tcb->smoothed_rtt * 7 / 8 + rtt / 8;
    }
    time_t new_rto_ms = MAX(1000, tcb->smoothed_rtt + MAX(TCP_RTO_GRANULARITY, 4 * tcb->rtt_variation));
    tcb->rto.tv_sec = new_rto_ms / 1000;
    tcb->rto.tv_nsec = new_rto_ms * 1000000;
    tcb->time_first_sent_valid = false;

#ifdef TCP_DEBUG
    debug_log("Took RTT sample: [ %ld, %ld, %ld, %ld ]\n", rtt, tcb->smoothed_rtt, tcb->rtt_variation, new_rto_ms);
#endif /* TCP_DEBUG */
}

static void tcp_advance_ack_number(struct socket *socket, uint32_t ack_number) {
    struct tcp_control_block *tcb = socket->private_data;
    uint32_t amount_to_advance = ack_number - tcb->send_unacknowledged;
    if (!amount_to_advance) {
        return;
    }

    if (tcb->pending_syn) {
        tcb->pending_syn = false;
        amount_to_advance--;
    }

    size_t total_pending_data = ring_buffer_size(&tcb->send_buffer);
    size_t data_acknowledged = MIN(amount_to_advance, total_pending_data);
    ring_buffer_advance(&tcb->send_buffer, data_acknowledged);
    amount_to_advance -= data_acknowledged;

    if (tcb->pending_fin && amount_to_advance > 0) {
        tcb->pending_fin = false;
        amount_to_advance--;
    }

    // Otherwise, the ACK should have been ingored as invalid.
    assert(amount_to_advance == 0);

    if (tcb->time_first_sent_valid && ack_number > tcb->time_first_sent_sequence_number) {
        tcp_sample_round_trip_time(socket);
    }

    tcb->send_unacknowledged = ack_number;

    if (tcb->state >= TCP_ESTABLISHED) {
        tcp_send_segments(socket);
    }

    if (tcb->send_unacknowledged == tcb->send_next) {
        struct timer *rto_timer = tcb->rto_timer;
        tcb->rto_timer = NULL;
        time_cancel_kernel_callback(rto_timer);
    } else {
        time_reset_kernel_callback(tcb->rto_timer, &tcb->rto);
    }

    socket->writable = true;
}

static void tcp_process_segment(struct socket *socket, const struct ip_v4_packet *ip_packet, const struct tcp_packet *packet) {
    struct tcp_control_block *tcb = socket->private_data;
    switch (tcb->state) {
        case TCP_SYN_SENT:
        case TCP_SYN_RECIEVED:
        case TCP_ESTABLISHED:
        case TCP_FIN_WAIT_1:
        case TCP_FIN_WAIT_2: {
            size_t segment_length = tcp_segment_length(ip_packet, packet);
            if (segment_length == 0) {
                return;
            }

            if (packet->flags.syn) {
                tcb->recv_next = htonl(packet->sequence_number);
                tcb->send_mss = tcp_mss(packet);
            }

            // Packet sent out of order, it would be best to remember it, but not for now.
            if (htonl(packet->sequence_number) > tcb->recv_next) {
                return;
            }

            size_t offset = tcb->recv_next - htonl(packet->sequence_number);
            segment_length -= offset + packet->flags.syn + packet->flags.fin;

            size_t available_space = ring_buffer_space(&tcb->recv_buffer);
            segment_length = MIN(available_space, segment_length);

            tcb->recv_next += packet->flags.syn + packet->flags.fin + segment_length;

            if (segment_length != 0) {
                ring_buffer_write(&tcb->recv_buffer, tcp_data_start(packet) + offset, segment_length);
                socket->readable = true;
            }

            bool window_changed = tcp_update_recv_window(socket);

            if (packet->flags.psh || packet->flags.fin || window_changed) {
                tcp_send_empty_ack(socket, ip_packet, packet);
            } else {
                tcb->send_ack_timer = time_register_kernel_callback(&ack_timeout, tcp_on_ack_timeout, socket);
            }
            break;
        }
        case TCP_CLOSE_WAIT:
        case TCP_CLOSING:
        case TCP_LAST_ACK:
        case TCP_TIME_WAIT:
            return;
        default:
            assert(false);
    }

    if (packet->flags.fin) {
        if (tcb->recv_next != htonl(packet->sequence_number) + tcp_segment_length(ip_packet, packet)) {
            return;
        }

        // All outstanding data has been recieved.
        socket->state = CLOSING;
        socket->readable = true;

        switch (tcb->state) {
            case TCP_SYN_SENT:
            case TCP_SYN_RECIEVED:
            case TCP_ESTABLISHED:
                tcb->state = TCP_CLOSE_WAIT;
                break;
            case TCP_FIN_WAIT_1:
                assert(tcb->pending_fin);
                tcb->state = TCP_CLOSING;
                break;
            case TCP_FIN_WAIT_2:
                tcb->state = TCP_TIME_WAIT;
                tcb->time_wait_timer = time_register_kernel_callback(&time_wait_timeout, tcp_time_wait_expiration, socket);
                break;
            case TCP_CLOSE_WAIT:
            case TCP_CLOSING:
            case TCP_LAST_ACK:
                break;
            case TCP_TIME_WAIT:
                time_reset_kernel_callback(tcb->time_wait_timer, &time_wait_timeout);
                break;
            default:
                assert(false);
        }
    }
}

static void tcp_recv_in_listen(struct socket *socket, const struct ip_v4_packet *ip_packet, const struct tcp_packet *packet) {
    struct tcp_control_block *tcb = socket->private_data;
    if (packet->flags.rst) {
        return;
    } else if (packet->flags.ack) {
        tcp_send_reset(ip_packet, packet);
        return;
    } else if (packet->flags.syn) {
        if (socket->num_pending >= socket->pending_length) {
            // There are too many pending connections, so refuse this one.
            tcp_send_reset(ip_packet, packet);
            return;
        }

        for (int i = 0; i < socket->num_pending; i++) {
            struct socket_connection *connection = socket->pending[i];
            if (connection->addr.in.sin_addr.s_addr == ip_v4_to_uint(ip_packet->source) &&
                connection->addr.in.sin_port == packet->source_port) {
                // This was a retransmitted SYN, no need to queue again.
                return;
            }
        }

        // Queue the connection
        struct socket_connection *connection = calloc(1, sizeof(struct socket_connection));
        connection->addr.in = (struct sockaddr_in) {
            .sin_family = AF_INET,
            .sin_port = packet->source_port,
            .sin_addr = { .s_addr = ip_v4_to_uint(ip_packet->source) },
            .sin_zero = { 0 },
        };
        connection->addrlen = sizeof(struct sockaddr_in);
        connection->connect_tcb = tcb;
        socket->pending[socket->num_pending++] = connection;
        socket->readable = true;

        // Update the TCB
        tcb->state = TCP_SYN_RECIEVED;
        tcp_process_segment(socket, ip_packet, packet);

        // Replace the TCB with a new one, for the next incoming connection
        struct tcp_control_block *new_tcb = net_allocate_tcp_control_block(socket);
        new_tcb->is_passive = true;
        new_tcb->state = TCP_LITSEN;
        return;
    }
}

static void tcp_recv_in_syn_sent(struct socket *socket, const struct ip_v4_packet *ip_packet, const struct tcp_packet *packet) {
    struct tcp_control_block *tcb = socket->private_data;
    if (packet->flags.ack) {
        if (htonl(packet->ack_number) <= tcb->send_unacknowledged || htonl(packet->ack_number) > tcb->send_next) {
            tcp_send_reset(ip_packet, packet);
            return;
        }
        if (packet->flags.rst) {
            socket->error = ECONNRESET;
            socket->state = CLOSED;
            net_free_tcp_control_block(socket);
            return;
        }
    } else if (packet->flags.rst) {
        return;
    }

    if (!packet->flags.syn) {
        return;
    }

    // Valid SYN was sent.
    socket->readable = true;
    if (packet->flags.ack) {
        tcp_advance_ack_number(socket, htonl(packet->ack_number));
        tcp_process_segment(socket, ip_packet, packet);
        tcp_enter_established_state(socket, packet);
        return;
    }

    tcb->state = TCP_SYN_RECIEVED;
    tcp_process_segment(socket, ip_packet, packet);
    return;
}

static void tcp_recv_data(struct socket *socket, const struct ip_v4_packet *ip_packet, const struct tcp_packet *packet) {
    struct tcp_control_block *tcb = socket->private_data;
    if (!tcp_segment_acceptable(socket, ip_packet, packet)) {
        tcp_send_empty_ack(socket, ip_packet, packet);
        return;
    }

    if (packet->flags.rst) {
        switch (tcb->state) {
            case TCP_SYN_RECIEVED: {
                if (tcb->is_passive) {
                    tcb->state = TCP_LITSEN;
                    return;
                }
                socket->error = ECONNREFUSED;
                socket->state = CLOSED;
                net_free_tcp_control_block(socket);
                return;
            }
            case TCP_ESTABLISHED:
            case TCP_FIN_WAIT_1:
            case TCP_FIN_WAIT_2:
            case TCP_CLOSE_WAIT:
                socket->error = ECONNRESET;
                socket->state = CLOSED;
                net_free_tcp_control_block(socket);
                return;
            case TCP_CLOSING:
            case TCP_LAST_ACK:
            case TCP_TIME_WAIT:
                net_free_tcp_control_block(socket);
                return;
            default:
                assert(false);
        }
    }

    if (packet->flags.syn) {
        tcp_send_reset(ip_packet, packet);
        socket->error = ECONNRESET;
        socket->state = CLOSED;
        net_free_tcp_control_block(socket);
        return;
    }

    if (!packet->flags.ack) {
        return;
    }

    switch (tcb->state) {
        case TCP_SYN_RECIEVED:
            if (tcb->send_unacknowledged > htonl(packet->ack_number) || htonl(packet->ack_number) > tcb->send_next) {
                tcp_send_reset(ip_packet, packet);
                break;
            }
            tcp_advance_ack_number(socket, htonl(packet->ack_number));
            tcp_process_segment(socket, ip_packet, packet);
            tcp_enter_established_state(socket, packet);
            return;
        case TCP_ESTABLISHED:
        case TCP_FIN_WAIT_1:
        case TCP_FIN_WAIT_2:
        case TCP_CLOSE_WAIT:
        case TCP_CLOSING:
        case TCP_LAST_ACK:
        case TCP_TIME_WAIT:
            if (htonl(packet->ack_number) <= tcb->send_unacknowledged) {
                break;
            } else if (htonl(packet->ack_number) > tcb->send_next) {
                tcp_send_empty_ack(socket, ip_packet, packet);
                return;
            }

            tcp_advance_ack_number(socket, htonl(packet->ack_number));
            if (tcb->send_wl1 < htonl(packet->sequence_number) ||
                (tcb->send_wl1 == htonl(packet->sequence_number) && tcb->send_wl2 <= htonl(packet->ack_number))) {
                tcp_update_send_window(socket, packet);
            }

            switch (tcb->state) {
                case TCP_FIN_WAIT_1:
                    if (tcb->pending_fin) {
                        break;
                    }
                    tcb->state = TCP_FIN_WAIT_2;
                    // Fall-through
                case TCP_FIN_WAIT_2:
                    socket->state = CLOSING;
                    break;
                case TCP_CLOSE_WAIT:
                    break;
                case TCP_CLOSING:
                    if (tcb->pending_fin) {
                        return;
                    }
                    tcb->state = TCP_TIME_WAIT;
                    tcb->time_wait_timer = time_register_kernel_callback(&time_wait_timeout, tcp_time_wait_expiration, socket);
                    break;
                case TCP_LAST_ACK:
                    assert(!tcb->pending_fin);
                    socket->state = CLOSED;
                    net_free_tcp_control_block(socket);
                    return;
                case TCP_TIME_WAIT:
                    tcp_send_empty_ack(socket, ip_packet, packet);
                    time_reset_kernel_callback(tcb->time_wait_timer, &time_wait_timeout);
                    break;
                default:
                    break;
            }
            break;
        default:
            assert(false);
    }

    tcp_process_segment(socket, ip_packet, packet);
}

static void tcp_recv_on_socket(struct socket *socket, const struct ip_v4_packet *ip_packet, const struct tcp_packet *packet) {
    struct tcp_control_block *tcb = socket->private_data;
    switch (tcb->state) {
        case TCP_LITSEN:
            tcp_recv_in_listen(socket, ip_packet, packet);
            return;
        case TCP_SYN_SENT:
            tcp_recv_in_syn_sent(socket, ip_packet, packet);
            return;
        case TCP_SYN_RECIEVED:
        case TCP_ESTABLISHED:
        case TCP_FIN_WAIT_1:
        case TCP_FIN_WAIT_2:
        case TCP_CLOSE_WAIT:
        case TCP_CLOSING:
        case TCP_LAST_ACK:
        case TCP_TIME_WAIT:
            tcp_recv_data(socket, ip_packet, packet);
            return;
        default:
            assert(false);
            break;
    }
}

void net_tcp_recieve(struct packet *net_packet) {
    struct packet_header *tcp_header = net_packet_inner_header(net_packet);
    struct tcp_packet *packet = tcp_header->raw_header;
    if (tcp_header->length < sizeof(struct tcp_packet)) {
        debug_log("TCP Packet to small\n");
        return;
    }

    net_tcp_log(net_packet);

    struct packet_header *ip_header = net_packet_innermost_header_of_type(net_packet, PH_IP_V4);
    assert(ip_header);

    struct ip_v4_packet *ip_packet = ip_header->raw_header;
    struct tcp_connection_info info = {
        .source_ip = IP_V4_ZEROES,
        .source_port = ntohs(packet->dest_port),
        .dest_ip = ip_packet->source,
        .dest_port = ntohs(packet->source_port),
    };

    struct socket *socket = net_get_tcp_socket_by_connection_info(&info);
    if (!socket) {
        // Check for a passive socket that doesn't know its own destination.
        info.dest_ip = IP_V4_ZEROES;
        info.dest_port = 0;
        socket = net_get_tcp_socket_by_connection_info(&info);
    }

    if (!socket) {
        info.dest_ip = ip_packet->source;
        info.dest_port = ntohs(packet->source_port);
        debug_log("TCP socket mapping not found: [ %d.%d.%d.%d, %u, %d.%d.%d.%d, %u ]\n", info.source_ip.addr[1], info.source_ip.addr[2],
                  info.source_ip.addr[3], info.source_ip.addr[0], info.source_port, info.dest_ip.addr[0], info.dest_ip.addr[1],
                  info.dest_ip.addr[2], info.dest_ip.addr[3], info.dest_port);
        tcp_send_reset(ip_packet, packet);
        return;
    }

    net_bump_socket(socket);
    mutex_lock(&socket->lock);
    tcp_recv_on_socket(socket, ip_packet, packet);
    mutex_unlock(&socket->lock);
    net_drop_socket(socket);
}

void net_init_tcp_packet(struct tcp_packet *packet, struct tcp_packet_options *opts) {
    packet->source_port = htons(opts->source_port);
    packet->dest_port = htons(opts->dest_port);
    packet->sequence_number = htonl(opts->sequence_number);
    packet->ack_number = htonl(opts->ack_number);
    packet->ns = 0;
    packet->zero = 0;
    packet->data_offset = (sizeof(struct tcp_packet) + opts->tcp_flags.syn * sizeof(struct tcp_option_mss)) / sizeof(uint32_t);
    packet->flags = opts->tcp_flags;
    packet->window_size = htons(opts->window);
    packet->check_sum = 0;
    packet->urg_pointer = 0;

    if (opts->tcp_flags.syn) {
        // Send an MSS option.
        struct tcp_option_mss *opt = (struct tcp_option_mss *) packet->options_and_payload;
        opt->type = TCP_OPTION_MSS;
        opt->length = sizeof(struct tcp_option_mss);
        opt->mss = htons(opts->mss);
    }

    if (opts->data_length > 0 && opts->data_rb != NULL) {
        ring_buffer_copy(opts->data_rb, opts->data_offset, (void *) tcp_data_start(packet), opts->data_length);
    }
}

void net_tcp_log(struct packet *net_packet) {
    struct tcp_packet *packet = net_packet->headers[net_packet->header_count - 1].raw_header;
    struct ip_v4_packet *ip_packet = net_packet->headers[net_packet->header_count - 2].raw_header;
    debug_log("TCP Packet:\n"
              "               Source Port  [ %15u ]   Dest Port [ %15u ]\n"
              "               Sequence     [ %15u ]   Ack Num   [ %15u ]\n"
              "               Win Size     [ %15u ]   Urg Point [ %15u ]\n"
              "               Source IP    [ %03u.%03u.%03u.%03u ]   Dest IP   [ %03u.%03u.%03u.%03u ]\n"
              "               Data Len     [ %15lu ]   Data off  [ %15u ]\n"
              "               Flags        [ FIN=%u SYN=%u RST=%u PSH=%u ACK=%u URG=%u ECE=%u CWR=%u ]\n",
              ntohs(packet->source_port), ntohs(packet->dest_port), ntohl(packet->sequence_number), ntohl(packet->ack_number),
              ntohs(packet->window_size), ntohs(packet->urg_pointer), ip_packet->source.addr[0], ip_packet->source.addr[1],
              ip_packet->source.addr[2], ip_packet->source.addr[3], ip_packet->destination.addr[0], ip_packet->destination.addr[1],
              ip_packet->destination.addr[2], ip_packet->destination.addr[3],
              ntohs(ip_packet->length) - sizeof(struct ip_v4_packet) - sizeof(uint32_t) * packet->data_offset, packet->data_offset,
              packet->flags.fin, packet->flags.syn, packet->flags.rst, packet->flags.psh, packet->flags.ack, packet->flags.urg,
              packet->flags.ece, packet->flags.cwr);
}
