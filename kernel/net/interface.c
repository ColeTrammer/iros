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
#include <kernel/net/socket.h>
#include <kernel/net/umessage.h>
#include <kernel/util/init.h>
#include <kernel/util/validators.h>

static struct list_node interface_list = INIT_LIST(interface_list);

static void add_interface(struct network_interface *interface) {
    list_append(&interface_list, &interface->interface_list);
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
        if (iter->type == NETWORK_INTERFACE_LOOPBACK && use_loopback && net_interface_ready(iter)) {
            interface = iter;
            break;
        } else if (iter->type == NETWORK_INTERFACE_ETHERNET && !use_loopback && net_interface_ready(iter)) {
            interface = iter;
            break;
        }
    }

#ifdef INTERFACE_DEBUG
    if (interface) {
        debug_log("Got interface: [ %s, %u.%u.%u.%u ]\n", interface->name, address.addr[0], address.addr[1], address.addr[2],
                  address.addr[3]);
    }
#endif /* INTERFACE_DEBUG */
    return interface;
}

struct network_interface *net_get_interface_for_socket(struct socket *socket, struct ip_v4_address destination) {
    if (socket->bound_interface) {
        return socket->bound_interface;
    }
    return net_get_interface_for_ip(destination);
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
        interface->link_layer_overhead = 0;
        interface->mtu = UINT16_MAX;
        interface->flags = IFF_UP | IFF_RUNNING | IFF_LOOPBACK;
    } else if (type == NETWORK_INTERFACE_ETHERNET) {
        interface->mask = (struct ip_v4_address) { { 255, 255, 255, 0 } };
        interface->link_layer_overhead = 1;
        interface->mtu = 1500;
        interface->flags = IFF_RUNNING | IFF_BROADCAST;
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

static int umessage_interface_recv(struct umessage_queue *queue, const struct umessage *umessage) {
    switch (umessage->type) {
        case UMESSAGE_INTERFACE_LIST_REQUEST: {
            if (!UMESSAGE_INTERFACE_LIST_REQUEST_VALID(umessage, umessage->length)) {
                return -EINVAL;
            }

            size_t interface_count = 0;
            net_for_each_interface(interface) {
                (void) interface;
                interface_count++;
            }

            struct queued_umessage *to_post =
                net_create_umessage(UMESSAGE_INTERFACE, UMESSAGE_INTERFACE_LIST, 0, UMESSAGE_INTERFACE_LIST_LENGTH(interface_count), NULL);
            struct umessage_interface_list *list = (void *) &to_post->message;
            list->interface_count = interface_count;
            int i = 0;
            net_for_each_interface(interface) {
                strcpy(list->interface_list[i].name, interface->name);
                list->interface_list[i].link_layer_address = interface->link_layer_address;
                list->interface_list[i].index = i + 1;
                list->interface_list[i].flags = interface->flags;
                i++;
            }

            net_post_umessage_to(queue, to_post);
            net_drop_umessage(to_post);
            return 0;
        }
        case UMESSAGE_INTERFACE_SET_STATE_REQUEST: {
            if (!UMESSAGE_INTERFACE_SET_STATE_REQUEST_VALID(umessage, umessage->length)) {
                return -EINVAL;
            }
            struct umessage_interface_set_state_request *req = (void *) umessage;

            int i = 0;
            struct network_interface *interface = NULL;
            net_for_each_interface(iter) {
                if (++i == req->interface_index) {
                    interface = iter;
                    break;
                }
            }

            if (!interface) {
                return -ENXIO;
            }

            if (req->set_subnet_mask) {
                interface->mask = ip_v4_from_uint(req->subnet_mask.s_addr);
            }
            if (req->set_default_gateway) {
                interface->default_gateway = ip_v4_from_uint(req->default_gateway.s_addr);
            }
            if (req->set_address) {
                interface->address = ip_v4_from_uint(req->address.s_addr);
            }
            if (req->set_flags) {
                interface->flags = req->flags;
            }
            return 0;
        }
        default:
            return -EINVAL;
    }
}

static struct umessage_category umessage_interface = {
    .category = UMESSAGE_INTERFACE,
    .request_type_count = UMESSAGE_INTERFACE_NUM_REQUESTS,
    .name = "UMessage Interface",
    .recv = umessage_interface_recv,
};

static void init_umessage_interface(void) {
    net_register_umessage_category(&umessage_interface);
}
INIT_FUNCTION(init_umessage_interface, net);
