#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include <kernel/hal/processor.h>
#include <kernel/hal/timer.h>
#include <kernel/net/ethernet.h>
#include <kernel/net/inet_socket.h>
#include <kernel/net/interface.h>
#include <kernel/net/ip.h>
#include <kernel/net/port.h>
#include <kernel/net/socket.h>
#include <kernel/net/socket_syscalls.h>
#include <kernel/net/tcp.h>
#include <kernel/net/udp.h>
#include <kernel/sched/task_sched.h>
#include <kernel/util/hash_map.h>

static struct hash_map *map;
static struct hash_map *server_map;

static struct socket_ops inet_ops = {
    .accept = net_inet_accept,
    .bind = net_inet_bind,
    .close = net_inet_close,
    .connect = net_inet_connect,
    .getpeername = net_inet_getpeername,
    .listen = net_inet_listen,
    .sendto = net_inet_sendto,
    .recvfrom = net_inet_recvfrom,
};

static unsigned int ip_v4_and_port_hash(void *i, int num_buckets) {
    struct ip_v4_and_port *a = i;
    return (a->ip.addr[0] + a->ip.addr[1] + a->ip.addr[2] + a->ip.addr[2] + a->port) % num_buckets;
}

static int ip_v4_and_port_equals(void *i1, void *i2) {
    return memcmp(i1, i2, sizeof(struct ip_v4_and_port)) == 0;
}

static void *ip_v4_and_port_key(void *m) {
    return &((struct tcp_socket_mapping *) m)->key;
}

HASH_DEFINE_FUNCTIONS(tcp_data, struct tcp_packet, uint32_t, sequence_number)

static void create_tcp_socket_mapping(struct socket *socket) {
    struct tcp_socket_mapping *mapping = calloc(1, sizeof(struct tcp_socket_mapping));
    mapping->key = (struct ip_v4_and_port) { PORT_FROM_SOCKADDR(&socket->peer_address), IP_V4_FROM_SOCKADDR(&socket->peer_address) };
    mapping->socket_id = socket->id;

    hash_put(map, mapping);
}

static void create_tcp_socket_mapping_for_source(struct socket *socket) {
    uint16_t source_port = PORT_FROM_SOCKADDR(&socket->host_address);
    struct ip_v4_address source_ip = IP_V4_FROM_SOCKADDR(&socket->host_address);

    struct tcp_socket_mapping *mapping = calloc(1, sizeof(struct tcp_socket_mapping));
    mapping->key = (struct ip_v4_and_port) { source_port, source_ip };
    mapping->socket_id = socket->id;

    debug_log("Created a tcp mapping: [ %u, %u.%u.%u.%u ]\n", source_port, source_ip.addr[0], source_ip.addr[1], source_ip.addr[2],
              source_ip.addr[3]);

    hash_put(server_map, mapping);
}

struct socket_data *net_inet_create_socket_data(const struct ip_v4_packet *packet, uint16_t port_network_ordered, const void *buf,
                                                size_t len) {
    struct socket_data *data = calloc(1, sizeof(struct socket_data) + len);
    assert(data);

    data->len = len;
    memcpy(data->data, buf, len);
    data->from.addrlen = sizeof(struct sockaddr_in);
    data->from.addr.in.sin_family = AF_INET;
    data->from.addr.in.sin_port = port_network_ordered;
    data->from.addr.in.sin_addr.s_addr = ip_v4_to_uint(packet->source);

    return data;
}

int net_inet_accept(struct socket *socket, struct sockaddr *addr, socklen_t *addrlen, int flags) {
    assert(socket);
    assert(socket->state == LISTENING);
    assert(socket->private_data);

    if (socket->protocol != IPPROTO_TCP) {
        return -EPROTONOSUPPORT;
    }

    struct socket_connection connection;
    int ret = net_get_next_connection(socket, &connection);
    if (ret != 0) {
        return ret;
    }

    if (addr) {
        assert(addrlen);
        memcpy(addr, &connection.addr, MIN(*addrlen, connection.addrlen));
        *addrlen = connection.addrlen;
    }

    debug_log("Creating connection: [ %lu ]\n", socket->id);

    int fd;
    struct socket *new_socket = net_create_socket_fd(AF_INET, (SOCK_STREAM & SOCK_TYPE_MASK) | flags, IPPROTO_TCP, &inet_ops, &fd, NULL);
    if (new_socket == NULL) {
        return fd;
    }

    new_socket->private_data = calloc(1, sizeof(struct inet_socket_data));
    struct inet_socket_data *new_data = new_socket->private_data;
    net_set_host_address(new_socket, &socket->host_address, sizeof(socket->host_address));
    net_set_peer_address(new_socket, &connection.addr.in, sizeof(connection.addr.in));

    struct tcp_control_block *tcb = calloc(1, sizeof(struct tcp_control_block));
    assert(tcb);
    new_data->tcb = tcb;
    tcb->should_send_ack = false;
    tcb->current_sequence_num = 0;
    tcb->current_ack_num = connection.ack_num + 1;
    tcb->sent_packets = hash_create_hash_map(tcp_data_hash, tcp_data_equals, tcp_data_key);
    assert(tcb->sent_packets);

    create_tcp_socket_mapping(new_socket);

    uint16_t source_port = PORT_FROM_SOCKADDR(&new_socket->host_address);
    struct ip_v4_address dest_ip = IP_V4_FROM_SOCKADDR(&new_socket->peer_address);
    uint16_t dest_port = PORT_FROM_SOCKADDR(&new_socket->peer_address);
    struct network_interface *interface = net_get_interface_for_ip(dest_ip);
    net_send_tcp(interface, dest_ip, source_port, dest_port, tcb->current_sequence_num, tcb->current_ack_num,
                 (union tcp_flags) { .bits.syn = 1, .bits.ack = 1 }, 0, NULL);

    // FIXME: we should wait for an ack
    new_socket->state = CONNECTED;

    return fd;
}

int net_inet_bind(struct socket *socket, const struct sockaddr *addr, socklen_t addrlen) {
    assert(socket);

    if (addr->sa_family != AF_INET || addrlen < sizeof(struct sockaddr_in)) {
        return -EINVAL;
    }

    struct inet_socket_data *data = calloc(1, sizeof(struct inet_socket_data));
    socket->private_data = data;

    uint16_t source_port = PORT_FROM_SOCKADDR(addr);
    if (source_port == 0) {
        int ret = net_bind_to_ephemeral_port(socket->id, &source_port);
        if (ret < 0) {
            return ret;
        }
    } else {
        int ret = net_bind_to_port(socket->id, source_port);
        if (ret < 0) {
            return ret;
        }
    }

    struct sockaddr_in host_address = {
        .sin_family = AF_INET, .sin_port = htons(source_port), .sin_addr = { ip_v4_to_uint(IP_V4_FROM_SOCKADDR(addr)) }, .sin_zero = { 0 }
    };
    net_set_host_address(socket, &host_address, sizeof(host_address));

    socket->state = BOUND;
    return 0;
}

static void __kill_tcp_data(void *a, void *i) {
    (void) i;
    free(a);
}

int net_inet_close(struct socket *socket) {
    assert(socket);

    struct inet_socket_data *data = socket->private_data;
    if (data) {
        struct ip_v4_address source_ip = IP_V4_FROM_SOCKADDR(&socket->host_address);
        uint16_t source_port = PORT_FROM_SOCKADDR(&socket->host_address);
        struct ip_v4_address dest_ip = IP_V4_FROM_SOCKADDR(&socket->peer_address);
        uint16_t dest_port = PORT_FROM_SOCKADDR(&socket->peer_address);
        net_unbind_port(source_port);
        if (socket->protocol == IPPROTO_TCP) {
            if (socket->state != LISTENING) {
                struct ip_v4_and_port key = { dest_port, dest_ip };
                free(hash_del(map, &key));
            } else {
                struct ip_v4_and_port key = { source_port, source_ip };
                free(hash_del(server_map, &key));
            }

            if (data->tcb) {
                assert(data->tcb->sent_packets);
                if (socket->state != CLOSED) {
                    struct network_interface *interface = net_get_interface_for_ip(dest_ip);
                    net_send_tcp(interface, dest_ip, source_port, dest_port, data->tcb->current_sequence_num, data->tcb->current_ack_num,
                                 (union tcp_flags) { .bits.fin = 1, .bits.ack = data->tcb->should_send_ack }, 0, NULL);
                }

                hash_for_each(data->tcb->sent_packets, __kill_tcp_data, NULL);
                hash_free_hash_map(data->tcb->sent_packets);
                free(data->tcb);
            }
        }

        free(data);
    }

    return 0;
}

int net_inet_connect(struct socket *socket, const struct sockaddr *addr, socklen_t addrlen) {
    assert(socket);

    if (socket->type != SOCK_STREAM || socket->protocol != IPPROTO_TCP || addrlen > sizeof(struct sockaddr_in)) {
        return -EINVAL;
    }

    if (addr->sa_family != AF_INET || addrlen < sizeof(struct sockaddr_in)) {
        return -EAFNOSUPPORT;
    }

    if (socket->state != BOUND) {
        struct sockaddr_in to_bind = { AF_INET, 0, { 0 }, { 0 } };
        int ret = net_inet_bind(socket, (struct sockaddr *) &to_bind, sizeof(struct sockaddr_in));
        if (ret < 0) {
            return ret;
        }
    }

    net_set_peer_address(socket, addr, addrlen);

    struct ip_v4_address dest_ip = IP_V4_FROM_SOCKADDR(addr);
    uint16_t dest_port = PORT_FROM_SOCKADDR(addr);
    struct network_interface *interface = net_get_interface_for_ip(dest_ip);

    create_tcp_socket_mapping(socket);
    assert(net_get_tcp_socket_by_ip_v4_and_port((struct ip_v4_and_port) { dest_port, dest_ip }) != NULL);

    struct inet_socket_data *data = socket->private_data;
    data->tcb = calloc(1, sizeof(struct tcp_control_block));
    assert(data->tcb);

    data->tcb->current_sequence_num = 0;
    data->tcb->current_ack_num = 0;

    data->tcb->sent_packets = hash_create_hash_map(tcp_data_hash, tcp_data_equals, tcp_data_key);
    assert(data->tcb->sent_packets);

    uint16_t source_port = PORT_FROM_SOCKADDR(&socket->host_address);
    net_send_tcp(interface, dest_ip, source_port, dest_port, data->tcb->current_sequence_num++, data->tcb->current_ack_num,
                 (union tcp_flags) { .raw_flags = TCP_FLAGS_SYN }, 0, NULL);

    for (;;) {
        if (socket->state == CONNECTED) {
            debug_log("Successfully connected socket: [ %lu ]\n", socket->id);
            return 0;
        }

        int ret = proc_block_until_socket_is_connected(get_current_task(), socket);
        if (ret) {
            return ret;
        }
    }

    return -ETIMEDOUT;
}

int net_inet_listen(struct socket *socket, int backlog) {
    assert(socket);
    if (socket->protocol != IPPROTO_TCP) {
        return -EPROTONOSUPPORT;
    }

    create_tcp_socket_mapping_for_source(socket);

    return net_generic_listen(socket, backlog);
}

int net_inet_getpeername(struct socket *socket, struct sockaddr *addr, socklen_t *addrlen) {
    int ret = 0;

    mutex_lock(&socket->lock);
    if (socket->has_peer_address) {
        net_copy_sockaddr_to_user(&socket->peer_address, sizeof(struct sockaddr_in), addr, addrlen);
    } else {
        ret = -ENOTCONN;
    }
    mutex_unlock(&socket->lock);

    return ret;
}

int net_inet_socket(int domain, int type, int protocol) {
    assert(domain == AF_INET);

    if (protocol == 0 && (type & SOCK_TYPE_MASK) == SOCK_DGRAM) {
        protocol = IPPROTO_UDP;
    }

    if (protocol == 0 && (type & SOCK_TYPE_MASK) == SOCK_STREAM) {
        protocol = IPPROTO_TCP;
    }

    if (protocol != IPPROTO_ICMP && protocol != IPPROTO_UDP && protocol != IPPROTO_TCP) {
        return -EPROTONOSUPPORT;
    }

    int fd;
    struct socket *socket = net_create_socket_fd(domain, type, protocol, &inet_ops, &fd, NULL);
    (void) socket;

    return fd;
}

ssize_t net_inet_sendto(struct socket *socket, const void *buf, size_t len, int flags, const struct sockaddr *dest, socklen_t addrlen) {
    (void) flags;

    assert(socket);
    assert((socket->type & SOCK_TYPE_MASK) == SOCK_RAW || (socket->type & SOCK_TYPE_MASK) == SOCK_DGRAM ||
           (socket->type & SOCK_TYPE_MASK) == SOCK_STREAM);

    if (dest && socket->type == SOCK_STREAM) {
        return -EINVAL;
    }

    if (!dest) {
        dest = (const struct sockaddr *) &socket->peer_address;
    }

    uint16_t source_port = PORT_FROM_SOCKADDR(&socket->host_address);
    struct ip_v4_address dest_ip = IP_V4_FROM_SOCKADDR(dest);
    uint16_t dest_port = PORT_FROM_SOCKADDR(dest);

    if (socket->protocol == IPPROTO_TCP) {
        assert(socket->state == CONNECTED);
        assert(socket->private_data);

        struct inet_socket_data *data = socket->private_data;
        struct network_interface *interface = net_get_interface_for_ip(dest_ip);

        net_send_tcp(interface, dest_ip, source_port, dest_port, data->tcb->current_sequence_num, data->tcb->current_ack_num,
                     (union tcp_flags) { .bits.ack = data->tcb->should_send_ack, .bits.fin = socket->state == CLOSING }, len, buf);

        data->tcb->should_send_ack = false;
        return (ssize_t) len;
    }

    assert(dest);
    if (dest->sa_family != AF_INET || addrlen < sizeof(struct sockaddr_in)) {
        return -EINVAL;
    }

    struct network_interface *interface = net_get_interface_for_ip(dest_ip);
    assert(interface);

    if ((socket->type & SOCK_TYPE_MASK) == SOCK_RAW) {
        return net_send_ip_v4(interface, socket->protocol, dest_ip, buf, len);
    }

    assert(socket->type == SOCK_DGRAM && socket->protocol == IPPROTO_UDP);
    if (socket->state != BOUND) {
        struct sockaddr_in to_bind = { AF_INET, 0, { 0 }, { 0 } };
        int ret = net_inet_bind(socket, (struct sockaddr *) &to_bind, sizeof(struct sockaddr_in));
        if (ret < 0) {
            return ret;
        }
    }

    if (dest == NULL) {
        return -EINVAL;
    }

    return net_send_udp(interface, dest_ip, source_port, dest_port, len, buf);
}

ssize_t net_inet_recvfrom(struct socket *socket, void *buf, size_t len, int flags, struct sockaddr *source, socklen_t *addrlen) {
    (void) flags;

    return net_generic_recieve_from(socket, buf, len, source, addrlen);
}

struct socket *net_get_tcp_socket_by_ip_v4_and_port(struct ip_v4_and_port tuple) {
    struct tcp_socket_mapping *mapping = hash_get(map, &tuple);
    if (mapping == NULL) {
        return NULL;
    }

    return net_get_socket_by_id(mapping->socket_id);
}

struct socket *net_get_tcp_socket_server_by_ip_v4_and_port(struct ip_v4_and_port tuple) {
    struct tcp_socket_mapping *mapping = hash_get(server_map, &tuple);
    if (mapping == NULL) {
        return NULL;
    }

    return net_get_socket_by_id(mapping->socket_id);
}

static struct socket_protocol tcp_protocol = {
    .domain = AF_INET,
    .type = SOCK_STREAM,
    .protocol = IPPROTO_TCP,
    .is_default_protocol = true,
    .name = "IPv4 TCP",
    .create_socket = net_inet_socket,
};

static struct socket_protocol udp_protocol = {
    .domain = AF_INET,
    .type = SOCK_DGRAM,
    .protocol = IPPROTO_UDP,
    .is_default_protocol = true,
    .name = "IPv4 UDP",
    .create_socket = net_inet_socket,
};

static struct socket_protocol icmp_protocol = {
    .domain = AF_INET,
    .type = SOCK_RAW,
    .protocol = IPPROTO_ICMP,
    .is_default_protocol = false,
    .name = "IPv4 ICMP",
    .create_socket = net_inet_socket,
};

void init_inet_sockets() {
    map = hash_create_hash_map(ip_v4_and_port_hash, ip_v4_and_port_equals, ip_v4_and_port_key);
    assert(map);

    server_map = hash_create_hash_map(ip_v4_and_port_hash, ip_v4_and_port_equals, ip_v4_and_port_key);
    assert(server_map);

    net_register_protocol(&tcp_protocol);
    net_register_protocol(&udp_protocol);
    net_register_protocol(&icmp_protocol);
}
