#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/hal/processor.h>
#include <kernel/net/inet_socket.h>
#include <kernel/net/socket_syscalls.h>
#include <kernel/net/tcp.h>
#include <kernel/net/tcp_socket.h>
#include <kernel/proc/blockers.h>
#include <kernel/proc/task.h>
#include <kernel/util/hash_map.h>

static int net_tcp_accept(struct socket *socket, struct sockaddr *addr, socklen_t *addrlen, int flags);
static int net_tcp_close(struct socket *socket);
static int net_tcp_connect(struct socket *socket, const struct sockaddr *addr, socklen_t addrlen);
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

static struct hash_map *tcp_sender_map;
static struct hash_map *tcp_server_map;

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

    hash_put(tcp_sender_map, mapping);
}

static void create_tcp_socket_mapping_for_source(struct socket *socket) {
    uint16_t source_port = PORT_FROM_SOCKADDR(&socket->host_address);
    struct ip_v4_address source_ip = IP_V4_FROM_SOCKADDR(&socket->host_address);

    struct tcp_socket_mapping *mapping = calloc(1, sizeof(struct tcp_socket_mapping));
    mapping->key = (struct ip_v4_and_port) { source_port, source_ip };
    mapping->socket_id = socket->id;

    debug_log("Created a tcp mapping: [ %u, %u.%u.%u.%u ]\n", source_port, source_ip.addr[0], source_ip.addr[1], source_ip.addr[2],
              source_ip.addr[3]);

    hash_put(tcp_server_map, mapping);
}

struct socket *net_get_tcp_socket_by_ip_v4_and_port(struct ip_v4_and_port tuple) {
    struct tcp_socket_mapping *mapping = hash_get(tcp_sender_map, &tuple);
    if (mapping == NULL) {
        return NULL;
    }

    return net_get_socket_by_id(mapping->socket_id);
}

struct socket *net_get_tcp_socket_server_by_ip_v4_and_port(struct ip_v4_and_port tuple) {
    struct tcp_socket_mapping *mapping = hash_get(tcp_server_map, &tuple);
    if (mapping == NULL) {
        return NULL;
    }

    return net_get_socket_by_id(mapping->socket_id);
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

    debug_log("Creating connection: [ %lu ]\n", socket->id);

    int fd;
    struct socket *new_socket = net_create_socket_fd(AF_INET, (SOCK_STREAM & SOCK_TYPE_MASK) | flags, IPPROTO_TCP, &tcp_ops, &fd, NULL);
    if (new_socket == NULL) {
        return fd;
    }

    net_set_peer_address(new_socket, &connection.addr.in, sizeof(connection.addr.in));
    net_set_host_address(new_socket, &socket->host_address, sizeof(socket->host_address));

    struct tcp_control_block *tcb = new_socket->private_data = calloc(1, sizeof(struct tcp_control_block));
    assert(tcb);
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

static void __kill_tcp_data(void *a, void *i) {
    (void) i;
    free(a);
}

static int net_tcp_close(struct socket *socket) {
    struct tcp_control_block *tcb = socket->private_data;
    if (tcb) {
        struct ip_v4_address source_ip = IP_V4_FROM_SOCKADDR(&socket->host_address);
        uint16_t source_port = PORT_FROM_SOCKADDR(&socket->host_address);
        struct ip_v4_address dest_ip = IP_V4_FROM_SOCKADDR(&socket->peer_address);
        uint16_t dest_port = PORT_FROM_SOCKADDR(&socket->peer_address);
        if (socket->state != LISTENING) {
            struct ip_v4_and_port key = { dest_port, dest_ip };
            free(hash_del(tcp_sender_map, &key));
        } else {
            struct ip_v4_and_port key = { source_port, source_ip };
            free(hash_del(tcp_server_map, &key));
        }

        assert(tcb->sent_packets);
        if (socket->state != CLOSED) {
            struct network_interface *interface = net_get_interface_for_ip(dest_ip);
            net_send_tcp(interface, dest_ip, source_port, dest_port, tcb->current_sequence_num, tcb->current_ack_num,
                         (union tcp_flags) { .bits.fin = 1, .bits.ack = tcb->should_send_ack }, 0, NULL);
        }

        hash_for_each(tcb->sent_packets, __kill_tcp_data, NULL);
        hash_free_hash_map(tcb->sent_packets);
        free(tcb);
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

    struct ip_v4_address dest_ip = IP_V4_FROM_SOCKADDR(addr);
    uint16_t dest_port = PORT_FROM_SOCKADDR(addr);
    struct network_interface *interface = net_get_interface_for_ip(dest_ip);

    create_tcp_socket_mapping(socket);
    assert(net_get_tcp_socket_by_ip_v4_and_port((struct ip_v4_and_port) { dest_port, dest_ip }) != NULL);

    struct tcp_control_block *tcb = socket->private_data = calloc(1, sizeof(struct tcp_control_block));
    assert(tcb);

    tcb->current_sequence_num = 0;
    tcb->current_ack_num = 0;

    tcb->sent_packets = hash_create_hash_map(tcp_data_hash, tcp_data_equals, tcp_data_key);
    assert(tcb->sent_packets);

    uint16_t source_port = PORT_FROM_SOCKADDR(&socket->host_address);
    net_send_tcp(interface, dest_ip, source_port, dest_port, tcb->current_sequence_num++, tcb->current_ack_num,
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

static int net_tcp_listen(struct socket *socket, int backlog) {
    create_tcp_socket_mapping_for_source(socket);
    return net_generic_listen(socket, backlog);
}

static int net_tcp_socket(int domain, int type, int protocol) {
    int fd;
    net_create_socket_fd(domain, type, protocol, &tcp_ops, &fd, NULL);
    return fd;
}

static ssize_t net_tcp_recvfrom(struct socket *socket, void *buf, size_t len, int flags, struct sockaddr *addr, socklen_t *addrlen) {
    (void) flags;

    int error = 0;
    struct socket_data *data = net_get_next_message(socket, &error);
    if (error) {
        return error;
    }

    struct tcp_control_block *tcb = socket->private_data;
    if (tcb->should_send_ack) {
        struct network_interface *interface = net_get_interface_for_ip(IP_V4_FROM_SOCKADDR(&socket->peer_address));

        net_send_tcp(interface, IP_V4_FROM_SOCKADDR(&socket->peer_address), PORT_FROM_SOCKADDR(&socket->host_address),
                     PORT_FROM_SOCKADDR(&socket->peer_address), tcb->current_sequence_num, tcb->current_ack_num,
                     (union tcp_flags) { .bits.ack = 1, .bits.fin = socket->state == CLOSING }, 0, NULL);
        tcb->should_send_ack = false;

        if (socket->state == CLOSING) {
            socket->state = CLOSED;
        }
    }

    size_t to_copy = MIN(len, data->len);
    memcpy(buf, data->data, to_copy);

    if (addr && addrlen) {
        net_copy_sockaddr_to_user(&data->from.addr, data->from.addrlen, addr, addrlen);
    }

    free(data);
    return (ssize_t) to_copy;
}

static ssize_t net_tcp_sendto(struct socket *socket, const void *buf, size_t len, int flags, const struct sockaddr *addr,
                              socklen_t addrlen) {
    (void) flags;

    if (!!addr || !!addrlen) {
        return -EINVAL;
    }

    uint16_t source_port = PORT_FROM_SOCKADDR(&socket->host_address);
    struct ip_v4_address dest_ip = IP_V4_FROM_SOCKADDR(&socket->peer_address);
    uint16_t dest_port = PORT_FROM_SOCKADDR(&socket->peer_address);

    assert(socket->state == CONNECTED);
    assert(socket->private_data);

    struct tcp_control_block *tcb = socket->private_data;
    struct network_interface *interface = net_get_interface_for_ip(dest_ip);

    net_send_tcp(interface, dest_ip, source_port, dest_port, tcb->current_sequence_num, tcb->current_ack_num,
                 (union tcp_flags) { .bits.ack = tcb->should_send_ack, .bits.fin = socket->state == CLOSING }, len, buf);

    tcb->should_send_ack = false;
    return (ssize_t) len;
}

void init_tcp_sockets(void) {
    tcp_sender_map = hash_create_hash_map(ip_v4_and_port_hash, ip_v4_and_port_equals, ip_v4_and_port_key);
    assert(tcp_sender_map);

    tcp_server_map = hash_create_hash_map(ip_v4_and_port_hash, ip_v4_and_port_equals, ip_v4_and_port_key);
    assert(tcp_server_map);

    net_register_protocol(&tcp_protocol);
}
