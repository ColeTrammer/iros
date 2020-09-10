#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <net/if.h>
#include <search.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/hal/output.h>
#include <kernel/net/arp.h>
#include <kernel/net/destination_cache.h>
#include <kernel/net/interface.h>
#include <kernel/net/ip.h>
#include <kernel/net/neighbor_cache.h>
#include <kernel/net/network_task.h>
#include <kernel/net/packet.h>
#include <kernel/util/validators.h>

static struct list_node interface_list = INIT_LIST(interface_list);

static void add_interface(struct network_interface *interface) {
    list_append(&interface_list, &interface->interface_list);
}

int net_interface_send_ip_v4(struct network_interface *self, struct destination_cache_entry *destination, struct packet *packet) {
    struct packet_header *outer_header = net_packet_outer_header(packet);

    struct ip_v4_packet *ip_packet = malloc(sizeof(struct ip_v4_packet));
    net_init_ip_v4_packet(ip_packet, destination ? destination->next_packet_id++ : 0, net_packet_header_to_ip_v4_type(outer_header->type),
                          packet->interface->address, destination ? destination->destination_path.dest_ip_address : IP_V4_BROADCAST, NULL,
                          packet->total_length);

    struct packet_header *ip_header =
        net_init_packet_header(packet, net_packet_header_index(packet, outer_header) - 1, PH_IP_V4, ip_packet, sizeof(struct ip_v4_packet));
    ip_header->flags |= PHF_DYNAMICALLY_ALLOCATED;

    if (!destination) {
        struct link_layer_address dest = self->ops->get_link_layer_broadcast_address(self);
        return self->ops->send(self, dest, packet);
    }

    return net_queue_packet_for_neighbor(destination->next_hop, packet);
}

void net_recieve_packet(struct network_interface *interface, struct packet *packet) {
    (void) interface;
    net_on_incoming_packet(packet);
}

struct list_node *net_get_interface_list(void) {
    return &interface_list;
}

struct network_interface *net_get_interface_for_ip(struct ip_v4_address address) {
    struct ip_v4_address loopback = IP_V4_LOOPBACK;
    bool use_loopback = memcmp(&address, &loopback, sizeof(struct ip_v4_address)) == 0;

    struct network_interface *interface = NULL;
    net_for_each_interface(iter) {
        if (iter->type == NETWORK_INTERFACE_LOOPBACK && use_loopback) {
            interface = iter;
            break;
        } else if (iter->type == NETWORK_INTERFACE_ETHERNET && !use_loopback) {
            interface = iter;
            break;
        }
    }

    assert(interface);

#ifdef INTERFACE_DEBUG
    debug_log("Got interface: [ %s, %u.%u.%u.%u ]\n", interface->name, address.addr[0], address.addr[1], address.addr[2], address.addr[3]);
#endif /* INTERFACE_DEBUG */
    return interface;
}

struct network_interface *net_create_network_interface(const char *name, int type, struct link_layer_address link_layer_address,
                                                       struct network_interface_ops *ops, void *data) {
    struct network_interface *interface = calloc(1, sizeof(struct network_interface));
    assert(interface);

    assert(name);
    strcpy(interface->name, name);

    assert(type == NETWORK_INTERFACE_ETHERNET || type == NETWORK_INTERFACE_LOOPBACK);
    interface->type = type;

    if (type == NETWORK_INTERFACE_LOOPBACK) {
        interface->address = IP_V4_LOOPBACK;
        interface->mask = IP_V4_BROADCAST;
        interface->default_gateway = IP_V4_LOOPBACK;
        interface->config_context.state = INITIALIZED;
        interface->link_layer_overhead = 0;
    } else if (type == NETWORK_INTERFACE_ETHERNET) {
        interface->link_layer_overhead = 1;
    }

    interface->link_layer_address = link_layer_address;

    assert(ops);
    interface->ops = ops;
    interface->private_data = data;

    add_interface(interface);

    return interface;
}

int net_ioctl_interface_index_for_name(struct ifreq *req) {
    if (validate_write(req, sizeof(*req))) {
        return -EFAULT;
    }

    int index = 1;
    net_for_each_interface(interface) {
        if (strcmp(interface->name, req->ifr_name) == 0) {
            req->ifr_ifindex = index;
            return 0;
        }
        index++;
    }
    return -ENODEV;
}

int net_ioctl_interface_name_for_index(struct ifreq *req) {
    if (validate_write(req, sizeof(*req))) {
        return -EFAULT;
    }

    int index = 1;
    net_for_each_interface(interface) {
        if (index == req->ifr_ifindex) {
            strcpy(req->ifr_name, interface->name);
            return 0;
        }
        index++;
    }
    return -ENXIO;
}
