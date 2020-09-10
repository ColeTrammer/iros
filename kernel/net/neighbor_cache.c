#include <assert.h>
#include <errno.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/hal/output.h>
#include <kernel/net/arp.h>
#include <kernel/net/destination_cache.h>
#include <kernel/net/interface.h>
#include <kernel/net/neighbor_cache.h>
#include <kernel/net/network_task.h>
#include <kernel/net/packet.h>
#include <kernel/net/socket.h>
#include <kernel/time/timer.h>

#define NEIGHBOR_CACHE_DEBUG

static struct hash_map *neighbor_cache;

static unsigned int neighbor_cache_entry_hash(void *_ip_address, int num_buckets) {
    struct ip_v4_address *ip_address = _ip_address;
    return (ip_address->addr[0] + ip_address->addr[1] + ip_address->addr[2] + ip_address->addr[3]) % num_buckets;
}

static int neighbor_cache_entry_equals(void *i1, void *i2) {
    return memcmp(i1, i2, sizeof(struct ip_v4_address)) == 0;
}

static void *neighbor_cache_entry_key(struct hash_entry *m) {
    return &hash_table_entry(m, struct neighbor_cache_entry)->ip_v4_address;
}

static struct neighbor_cache_entry *create_neighbor_cache_entry(struct ip_v4_address address) {
    struct neighbor_cache_entry *neighbor = malloc(sizeof(struct neighbor_cache_entry));
    init_list(&neighbor->queued_packets);
    neighbor->update_timer = NULL;
    neighbor->ref_count = 1;
    init_spinlock(&neighbor->lock);
    neighbor->ip_v4_address = address;
    neighbor->state = NS_INCOMPLETE;
    neighbor->link_layer_address = LINK_LAYER_ADDRESS_NONE;
    return neighbor;
}

struct neighbor_cache_entry *net_bump_neighbor_cache_entry(struct neighbor_cache_entry *neighbor) {
    atomic_fetch_add(&neighbor->ref_count, 1);
    return neighbor;
}

void net_drop_neighbor_cache_entry(struct neighbor_cache_entry *neighbor) {
    if (atomic_fetch_sub(&neighbor->ref_count, 1) == 1) {
        if (neighbor->update_timer) {
            time_cancel_kernel_callback(neighbor->update_timer);
            neighbor->update_timer = NULL;
        }
        assert(list_is_empty(&neighbor->queued_packets));
        free(neighbor);
    }
}

static void neighbor_lookup_failed(struct timer *timer __attribute__((unused)), void *_neighbor) {
    struct neighbor_cache_entry *neighbor = _neighbor;

    spin_lock(&neighbor->lock);
    time_cancel_kernel_callback(neighbor->update_timer);
    neighbor->update_timer = NULL;

    list_for_each_entry_safe(&neighbor->queued_packets, packet, struct packet, queue) {
        if (packet->socket) {
            net_socket_set_error(packet->socket, -EHOSTUNREACH);
        }
        list_remove(&packet->queue);
        net_free_packet(packet);
    }
    neighbor->state = NS_UNREACHABLE;
    spin_unlock(&neighbor->lock);

    net_remove_neighbor(neighbor);
}

int net_queue_packet_for_neighbor(struct neighbor_cache_entry *neighbor, struct packet *packet) {
    int ret = 0;
    spin_lock(&neighbor->lock);
    if (neighbor->state == NS_UNREACHABLE) {
        // This host has proven to be unreachable. No new connections will use this neighbor cache entry, but old
        // ones might still be holding on it, and should be terminated.
        ret = -EHOSTUNREACH;
        net_free_packet(packet);
        goto done;
    }

    if (neighbor->state != NS_INCOMPLETE) {
        // In this case, no link layer address lookup needs to be performed, and as such, the data
        // can be sent right away.
        ret = packet->interface->ops->send(packet->interface, neighbor->link_layer_address, packet);
        goto done;
    }

    if (!neighbor->update_timer) {
        // There is no update timer specified, which means no ARP request has been sent yet. Time to make an ARP request and then start
        // a timeout.
        struct timespec delay = { .tv_sec = 0, .tv_nsec = 500000000 };
        neighbor->update_timer = time_register_kernel_callback(&delay, neighbor_lookup_failed, neighbor);
        net_send_arp_request(packet->interface, neighbor->ip_v4_address);
    }

    // Queue the packet for later processing. This should probably enforce a limit on the number of packets queued.
    list_append(&neighbor->queued_packets, &packet->queue);

done:
    spin_unlock(&neighbor->lock);
    return ret;
}

struct neighbor_cache_entry *net_lookup_neighbor(struct ip_v4_address address) {
    struct neighbor_cache_entry *neighbor = hash_get_entry(neighbor_cache, &address, struct neighbor_cache_entry);
    if (!neighbor) {
        neighbor = create_neighbor_cache_entry(address);
        hash_put(neighbor_cache, &neighbor->hash);
    }
    return net_bump_neighbor_cache_entry(neighbor);
}

void net_update_neighbor(struct neighbor_cache_entry *neighbor, struct link_layer_address confirmed_address) {
    spin_lock(&neighbor->lock);
    if (neighbor->update_timer) {
        time_cancel_kernel_callback(neighbor->update_timer);
        neighbor->update_timer = NULL;
    }

    neighbor->state = NS_REACHABLE;
    neighbor->link_layer_address = confirmed_address;

#ifdef NEIGHBOR_CACHE_DEBUG
    debug_log("Confirmed link layer address of neighbor: [ %d.%d.%d.%d, %02x:%02x:%02x:%02x:%02x:%02x ]\n", neighbor->ip_v4_address.addr[0],
              neighbor->ip_v4_address.addr[1], neighbor->ip_v4_address.addr[2], neighbor->ip_v4_address.addr[3],
              neighbor->link_layer_address.addr[0], neighbor->link_layer_address.addr[1], neighbor->link_layer_address.addr[2],
              neighbor->link_layer_address.addr[3], neighbor->link_layer_address.addr[4], neighbor->link_layer_address.addr[5]);
#endif /* NEIGHBOR_CACHE_DEBUG */

    list_for_each_entry_safe(&neighbor->queued_packets, packet, struct packet, queue) {
        list_remove(&packet->queue);

        struct socket *socket = packet->socket;
        int ret = packet->interface->ops->send(packet->interface, neighbor->link_layer_address, packet);
        if (!!ret && !!socket) {
            net_socket_set_error(socket, ret);
        }
    }

    spin_unlock(&neighbor->lock);
}

static void remove_stale_destinations(struct hash_entry *hash, void *neighbor) {
    struct destination_cache_entry *destination = hash_table_entry(hash, struct destination_cache_entry);
    if (destination->next_hop == neighbor) {
        __net_remove_destination(destination);
    }
}

void net_remove_neighbor(struct neighbor_cache_entry *neighbor) {
#ifdef NEIGHBOR_CACHE_DEBUG
    debug_log("Removing neighbor cache entry: [ %d.%d.%d.%d ]\n", neighbor->ip_v4_address.addr[0], neighbor->ip_v4_address.addr[1],
              neighbor->ip_v4_address.addr[2], neighbor->ip_v4_address.addr[3]);
#endif /* NEIGHBOR_CACHE_DEBUG */

    hash_del(neighbor_cache, &neighbor->ip_v4_address);
    net_drop_neighbor_cache_entry(neighbor);
    hash_for_each(net_destination_cache(), remove_stale_destinations, neighbor);
}

struct hash_map *net_neighbor_cache(void) {
    return neighbor_cache;
}

void init_neighbor_cache(void) {
    neighbor_cache = hash_create_hash_map(neighbor_cache_entry_hash, neighbor_cache_entry_equals, neighbor_cache_entry_key);
}
