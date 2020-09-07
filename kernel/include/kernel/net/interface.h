#ifndef _KERNEL_NET_INTERFACE_H
#define _KERNEL_NET_INTERFACE_H 1

#include <sys/types.h>

#include <kernel/net/ip_address.h>
#include <kernel/net/mac.h>
#include <kernel/util/list.h>

struct arp_packet;
struct ifreq;
struct ip_v4_packet;
struct network_data;
struct network_interface;
struct destination_cache_entry;

struct network_interface_ops {
    int (*send_ethernet)(struct network_interface *interface, struct mac_address dest, uint16_t ether_type, const void *data,
                         uint16_t data_len);
    int (*send_arp)(struct network_interface *interface, struct link_layer_address dest, struct network_data *data);
    int (*send_ip_v4)(struct network_interface *interface, struct destination_cache_entry *destination, struct network_data *data);
    struct link_layer_address (*get_link_layer_address)(struct network_interface *interface);
    struct link_layer_address (*get_link_layer_broadcast_address)(struct network_interface *interface);
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

void net_recieve_network_data(struct network_interface *interface, struct network_data *data);

int net_ioctl_interface_index_for_name(struct ifreq *req);
int net_ioctl_interface_name_for_index(struct ifreq *req);

#define net_for_each_interface(name) list_for_each_entry(net_get_interface_list(), name, struct network_interface, interface_list)

#endif /* _KERNEL_NET_INTERFACE_H */
