#ifndef _KERNEL_NET_INTERFACE_H
#define _KERNEL_NET_INTERFACE_H 1

#include <net/if.h>
#include <sys/types.h>

#include <kernel/net/ip_address.h>
#include <kernel/net/link_layer_address.h>
#include <kernel/util/list.h>

struct destination_cache_entry;
struct ifreq;
struct network_interface;
struct packet;
struct socket;

struct network_interface_ops {
    int (*send)(struct network_interface *interface, struct link_layer_address dest, struct packet *packet);
    int (*send_ip_v4)(struct network_interface *interface, struct link_layer_address dest, struct packet *packet);
    int (*route_ip_v4)(struct network_interface *interface, struct packet *packet);
    struct link_layer_address (*get_link_layer_broadcast_address)(struct network_interface *interface);
};

struct network_interface {
    struct list_node interface_list;

    char name[8];
#define NETWORK_INTERFACE_ETHERNET 1
#define NETWORK_INTERFACE_LOOPBACK 2
    int type;
    int flags;

    uint32_t link_layer_overhead;
    uint16_t mtu;

    struct ip_v4_address address;
    struct ip_v4_address mask;
    struct ip_v4_address default_gateway;

    struct link_layer_address link_layer_address;

    struct network_interface_ops *ops;

    void *private_data;
};

struct list_node *net_get_interface_list(void);
struct network_interface *net_get_interface_for_ip(struct ip_v4_address address);
struct network_interface *net_get_interface_for_socket(struct socket *socket, struct ip_v4_address destination);
struct network_interface *net_create_network_interface(const char *name, int type, struct link_layer_address link_layer_address,
                                                       struct network_interface_ops *ops, void *data);

void net_recieve_packet(struct network_interface *interface, struct packet *packet);

int net_ioctl_interface_index_for_name(struct ifreq *req);
int net_ioctl_interface_name_for_index(struct ifreq *req);

static inline bool net_interface_ready(struct network_interface *interface) {
    return !!(interface->flags & IFF_UP) && !!(interface->flags & IFF_RUNNING);
}

#define net_for_each_interface(name) list_for_each_entry(net_get_interface_list(), name, struct network_interface, interface_list)

#endif /* _KERNEL_NET_INTERFACE_H */
