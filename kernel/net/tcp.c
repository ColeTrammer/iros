#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/hal/output.h>
#include <kernel/net/ethernet.h>
#include <kernel/net/inet_socket.h>
#include <kernel/net/interface.h>
#include <kernel/net/ip.h>
#include <kernel/net/route_cache.h>
#include <kernel/net/tcp.h>
#include <kernel/net/tcp_socket.h>
#include <kernel/time/timer.h>
#include <kernel/util/macros.h>

int net_send_tcp_from_socket(struct socket *socket) {
    uint16_t source_port = PORT_FROM_SOCKADDR(&socket->host_address);
    struct ip_v4_address dest_ip = IP_V4_FROM_SOCKADDR(&socket->peer_address);
    uint16_t dest_port = PORT_FROM_SOCKADDR(&socket->peer_address);

    struct tcp_control_block *tcb = socket->private_data;

    size_t data_to_send = tcb->send_next - tcb->send_unacknowledged - tcb->pending_syn - tcb->pending_fin;

    int ret =
        net_send_tcp(dest_ip, source_port, dest_port, tcb->send_unacknowledged, tcb->recv_next, tcb->recv_window,
                     (union tcp_flags) { .bits = { .ack = tcb->state != TCP_SYN_SENT, .syn = tcb->pending_syn, .fin = tcb->pending_fin } },
                     data_to_send, &tcb->send_buffer);
    if (ret) {
        return ret;
    }

    return 0;
}

int net_send_tcp(struct ip_v4_address dest, uint16_t source_port, uint16_t dest_port, uint32_t sequence_number, uint32_t ack_num,
                 uint16_t window, union tcp_flags flags, uint16_t len, struct ring_buffer *rb) {
    struct network_interface *interface = net_get_interface_for_ip(dest);
    if (interface->config_context.state != INITIALIZED) {
        debug_log("Can't send TCP packet; interface uninitialized: [ %s ]\n", interface->name);
        return -ENETDOWN;
    }

    struct route_cache_entry *route = net_find_next_hop_gateway(interface, dest);
    size_t total_length = sizeof(struct ip_v4_packet) + sizeof(struct tcp_packet) + len;

    struct ip_v4_packet *ip_packet =
        net_create_ip_v4_packet(1, IP_V4_PROTOCOL_TCP, interface->address, dest, NULL, total_length - sizeof(struct ip_v4_packet));

    struct tcp_packet *tcp_packet = (struct tcp_packet *) ip_packet->payload;
    net_init_tcp_packet(tcp_packet, source_port, dest_port, sequence_number, ack_num, flags, window, len, rb);

    struct ip_v4_pseudo_header header = { interface->address, dest, 0, IP_V4_PROTOCOL_TCP, htons(len + sizeof(struct tcp_packet)) };

    tcp_packet->check_sum = ntohs(in_compute_checksum_with_start(tcp_packet, sizeof(struct tcp_packet) + len,
                                                                 in_compute_checksum(&header, sizeof(struct ip_v4_pseudo_header))));

    if (interface->type != NETWORK_INTERFACE_LOOPBACK) {
        debug_log("Sending TCP packet\n");
        net_tcp_log(ip_packet, tcp_packet);
    }

    int ret = interface->ops->send_ip_v4(interface, route, ip_packet, total_length);
    free(ip_packet);

    net_drop_route_cache_entry(route);
    return ret;
}

static size_t tcp_segment_length(const struct ip_v4_packet *ip_packet, const struct tcp_packet *packet) {
    return ntohs(ip_packet->length) - sizeof(struct ip_v4_packet) - sizeof(uint32_t) * packet->data_offset + packet->flags.bits.syn +
           packet->flags.bits.fin;
}

static const void *tcp_data_start(const struct tcp_packet *packet) {
    return (const void *) packet + sizeof(uint32_t) * packet->data_offset;
}

static void tcp_send_reset(const struct ip_v4_packet *ip_packet, const struct tcp_packet *packet) {
    if (packet->flags.bits.rst) {
        return;
    }

    if (packet->flags.bits.ack) {
        net_send_tcp(ip_packet->source, htons(packet->dest_port), htons(packet->source_port), htonl(packet->ack_number), 0, 0,
                     (union tcp_flags) { .bits = { .rst = 1 } }, 0, NULL);
    } else {
        net_send_tcp(ip_packet->source, htons(packet->dest_port), htons(packet->source_port), 0,
                     htonl(packet->sequence_number) + tcp_segment_length(ip_packet, packet), 0,
                     (union tcp_flags) { .bits = { .ack = 1, .rst = 1 } }, 0, NULL);
    }
}

static void tcp_send_empty_ack(struct socket *socket, const struct ip_v4_packet *ip_packet, const struct tcp_packet *packet) {
    if (packet->flags.bits.rst) {
        return;
    }

    struct tcp_control_block *tcb = socket->private_data;

    net_send_tcp(ip_packet->source, htons(packet->dest_port), htons(packet->source_port), tcb->send_next, tcb->recv_next, tcb->recv_window,
                 (union tcp_flags) { .bits = { .ack = 1 } }, 0, NULL);
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

static void tcp_advance_ack_number(struct socket *socket, uint32_t ack_number) {
    struct tcp_control_block *tcb = socket->private_data;
    uint32_t amount_to_advance = ack_number - tcb->send_unacknowledged;

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

    tcb->send_unacknowledged = ack_number;
    if (tcb->send_unacknowledged == tcb->send_next) {
        struct timer *retransmission_timer = tcb->retransmission_timer;
        tcb->retransmission_timer = NULL;
        time_cancel_kernel_callback(retransmission_timer);
    }

    socket->writable = true;
}

static void tcp_process_segment(struct socket *socket, const struct ip_v4_packet *ip_packet, const struct tcp_packet *packet) {
    struct tcp_control_block *tcb = socket->private_data;
    switch (tcb->state) {
        case TCP_SYN_RECIEVED:
        case TCP_ESTABLISHED:
        case TCP_FIN_WAIT_1:
        case TCP_FIN_WAIT_2: {
            size_t segment_length = tcp_segment_length(ip_packet, packet);
            size_t offset = 0;

            // Packet sent out of order, it would be best to remember it, but not for now.
            if (htonl(packet->sequence_number) > tcb->recv_next) {
                return;
            }

            offset = tcb->recv_next - htonl(packet->sequence_number);
            segment_length -= offset + packet->flags.bits.syn + packet->flags.bits.fin;
            tcb->recv_next += packet->flags.bits.syn;

            size_t available_space = ring_buffer_space(&tcb->recv_buffer);
            segment_length = MIN(available_space, segment_length);
            if (segment_length == 0) {
                break;
            }

            ring_buffer_write(&tcb->recv_buffer, tcp_data_start(packet) + offset, segment_length);
            socket->readable = true;
            tcb->recv_next += segment_length;
            tcb->recv_window -= segment_length;
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

    if (packet->flags.bits.fin) {
        if (tcb->recv_next != htonl(packet->sequence_number) + tcp_segment_length(ip_packet, packet) - 1) {
            return;
        }

        // All outstanding data has been recieved.
        tcb->recv_next++;
        tcb->state = CLOSING;
        socket->readable = true;

        net_tcp_send_segment(socket);
        switch (tcb->state) {
            case TCP_SYN_RECIEVED:
            case TCP_ESTABLISHED:
                tcb->state = TCP_CLOSE_WAIT;
                break;
            case TCP_FIN_WAIT_1:
                assert(!tcb->pending_fin);
                tcb->state = CLOSING;
                break;
            case TCP_FIN_WAIT_2:
                tcb->state = TCP_TIME_WAIT;
                break;
            case TCP_CLOSE_WAIT:
            case TCP_CLOSING:
            case TCP_LAST_ACK:
                break;
            case TCP_TIME_WAIT:
                // FIXME: restart the 2 MSL timeout
                break;
            default:
                assert(false);
        }
    }
}

static void tcp_recv_in_listen(struct socket *socket, const struct ip_v4_packet *ip_packet, const struct tcp_packet *packet) {
    struct tcp_control_block *tcb = socket->private_data;
    if (packet->flags.bits.rst) {
        return;
    } else if (packet->flags.bits.ack) {
        tcp_send_reset(ip_packet, packet);
        return;
    } else if (packet->flags.bits.syn) {
        (void) tcb;
        debug_log("Got SYN\n");
        return;
    }
}

static void tcp_recv_in_syn_sent(struct socket *socket, const struct ip_v4_packet *ip_packet, const struct tcp_packet *packet) {
    struct tcp_control_block *tcb = socket->private_data;
    if (packet->flags.bits.ack) {
        if (htonl(packet->ack_number) <= tcb->send_unacknowledged || htonl(packet->ack_number) > tcb->send_next) {
            tcp_send_reset(ip_packet, packet);
            return;
        }
        if (packet->flags.bits.rst) {
            socket->error = ECONNRESET;
            socket->state = CLOSED;
            net_free_tcp_control_block(socket);
            return;
        }
    } else if (packet->flags.bits.rst) {
        return;
    }

    if (!packet->flags.bits.syn) {
        return;
    }

    // Valid SYN was sent.
    tcb->recv_next = htonl(packet->sequence_number);
    tcb->send_window = htons(packet->window_size);
    socket->readable = true;
    if (packet->flags.bits.ack) {
        tcp_advance_ack_number(socket, htonl(packet->ack_number));
        tcb->state = TCP_ESTABLISHED;
        net_tcp_send_segment(socket);
        tcp_process_segment(socket, ip_packet, packet);
        return;
    }

    tcb->state = TCP_SYN_RECIEVED;
    net_tcp_send_segment(socket);
    tcp_process_segment(socket, ip_packet, packet);
    return;
}

static void tcp_recv_data(struct socket *socket, const struct ip_v4_packet *ip_packet, const struct tcp_packet *packet) {
    struct tcp_control_block *tcb = socket->private_data;
    if (!tcp_segment_acceptable(socket, ip_packet, packet)) {
        tcp_send_empty_ack(socket, ip_packet, packet);
        return;
    }

    if (packet->flags.bits.rst) {
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

    if (packet->flags.bits.syn) {
        tcp_send_reset(ip_packet, packet);
        socket->error = ECONNRESET;
        socket->state = CLOSED;
        net_free_tcp_control_block(socket);
        return;
    }

    if (!packet->flags.bits.ack) {
        return;
    }

    switch (tcb->state) {
        case TCP_SYN_RECIEVED:
            if (tcb->send_unacknowledged > htonl(packet->ack_number) || htonl(packet->ack_number) > tcb->send_next) {
                tcp_send_reset(ip_packet, packet);
                break;
            }
            tcb->state = TCP_ESTABLISHED;
            // Fall-through
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
                tcb->send_window = htons(packet->window_size);
                tcb->send_wl1 = htonl(packet->sequence_number);
                tcb->send_wl2 = htonl(packet->ack_number);
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
                    break;
                case TCP_LAST_ACK:
                    assert(!tcb->pending_fin);
                    socket->state = CLOSED;
                    net_free_tcp_control_block(socket);
                    break;
                case TCP_TIME_WAIT:
                    tcp_send_empty_ack(socket, ip_packet, packet);
                    // FIXME: restart the 2 MSL timeout.
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

void net_tcp_recieve(const struct ip_v4_packet *ip_packet, const struct tcp_packet *packet, size_t len) {
    assert(packet);
    if (len < sizeof(struct tcp_packet)) {
        debug_log("TCP Packet to small\n");
        return;
    }

    net_tcp_log(ip_packet, packet);

    struct tcp_connection_info info = {
        .source_ip = IP_V4_BROADCAST,
        .source_port = ntohs(packet->dest_port),
        .dest_ip = ip_packet->source,
        .dest_port = ntohs(packet->source_port),
    };

    struct socket *socket = net_get_tcp_socket_by_connection_info(&info);
    if (!socket) {
        debug_log("TCP socket mapping not found: [ %d.%d.%d.%d, %u, %d.%d.%d.%d, %u ]\n", info.source_ip.addr[0], info.source_ip.addr[0],
                  info.source_ip.addr[0], info.source_ip.addr[0], info.source_port, info.dest_ip.addr[0], info.dest_ip.addr[0],
                  info.dest_ip.addr[0], info.dest_ip.addr[0], info.dest_port);
        tcp_send_reset(ip_packet, packet);
        return;
    }

    mutex_lock(&socket->lock);
    tcp_recv_on_socket(socket, ip_packet, packet);
    mutex_unlock(&socket->lock);
}

void net_init_tcp_packet(struct tcp_packet *packet, uint16_t source_port, uint16_t dest_port, uint32_t sequence, uint32_t ack_num,
                         union tcp_flags flags, uint16_t win_size, uint16_t payload_length, struct ring_buffer *rb) {
    packet->source_port = htons(source_port);
    packet->dest_port = htons(dest_port);
    packet->sequence_number = htonl(sequence);
    packet->ack_number = htonl(ack_num);
    packet->ns = 0;
    packet->zero = 0;
    packet->data_offset = sizeof(struct tcp_packet) / sizeof(uint32_t);
    packet->flags = flags;
    packet->window_size = htons(win_size);
    packet->check_sum = 0;
    packet->urg_pointer = 0;

    if (payload_length > 0 && rb != NULL) {
        ring_buffer_copy(rb, packet->options_and_payload, payload_length);
    }
}

void net_tcp_log(const struct ip_v4_packet *ip_packet, const struct tcp_packet *packet) {
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
              packet->flags.bits.fin, packet->flags.bits.syn, packet->flags.bits.rst, packet->flags.bits.psh, packet->flags.bits.ack,
              packet->flags.bits.urg, packet->flags.bits.ece, packet->flags.bits.cwr);
}
