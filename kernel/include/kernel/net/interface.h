#ifndef _KERNEL_NET_INTERFACE_H
#define _KERNEL_NET_INTERFACE_H 1

#include <sys/types.h>

#include <kernel/net/ip_address.h>
#include <kernel/net/mac.h>
#include <kernel/util/list.h>

struct arp_packet;
struct ethernet_frame;
struct ip_v4_packet;
struct network_interface;
struct route_cache_entry;

struct network_interface_ops {
    int (*send_arp)(struct network_interface *interface, struct mac_address dest, const struct arp_packet *data, size_t len);
    int (*send_ip_v4)(struct network_interface *interface, struct route_cache_entry *route, const struct ip_v4_packet *data, size_t len);
    void (*recieve_ethernet)(struct network_interface *interface, const struct ethernet_frame *frame, size_t len);
    void (*recieve_ip_v4_sync)(struct network_interface *interface, const struct ip_v4_packet *packet, size_t len);
    struct mac_address (*get_mac_address)(struct network_interface *interface);
};

enum network_configuration_state { UNINITALIZED = 0, INITIALIZED };

struct network_configuration_context {
    uint32_t xid;
    enum network_configuration_state state;
};

struct network_interface {
    struct list_node interface_list;

    char name[8];
#define NETWORK_INTERFACE_ETHERNET 1
#define NETWORK_INTERFACE_LOOPBACK 2
    int type;

    struct ip_v4_address address;
    struct ip_v4_address mask;
    struct ip_v4_address default_gateway;

    struct network_configuration_context config_context;

    struct network_interface_ops *ops;

    void *private_data;
};

struct list_node *net_get_interface_list(void);
struct network_interface *net_get_interface_for_ip(struct ip_v4_address address);
struct network_interface *net_create_network_interface(const char *name, int type, struct network_interface_ops *ops, void *data);

#define net_for_each_interface(name) list_for_each_entry(net_get_interface_list(), name, struct network_interface, interface_list)

#endif /* _KERNEL_NET_INTERFACE_H */
