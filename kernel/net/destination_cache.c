#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/hal/output.h>
#include <kernel/net/destination_cache.h>
#include <kernel/net/interface.h>
#include <kernel/net/neighbor_cache.h>
#include <kernel/util/random.h>

#define DESTINATION_CACHE_DEBUG

static struct hash_map *destination_cache;

static struct destination_cache_entry loopback_route = {
    .destination_path = {
        .local_ip_address = IP_V4_LOOPBACK,
        .dest_ip_address = IP_V4_LOOPBACK,
    },
    .next_hop = NULL,
    .hash = { 0 },
    .ref_count = 1,
};

static unsigned int destination_cache_entry_hash(void *_path, int num_buckets) {
    struct destination_path *path = _path;
    return (path->local_ip_address.addr[0] + path->local_ip_address.addr[1] + path->local_ip_address.addr[2] +
            path->local_ip_address.addr[3] + path->dest_ip_address.addr[0] + path->dest_ip_address.addr[1] + path->dest_ip_address.addr[2] +
            path->dest_ip_address.addr[3]) %
           num_buckets;
}

static int destination_cache_entry_equals(void *i1, void *i2) {
    return memcmp(i1, i2, sizeof(struct destination_path)) == 0;
}

static void *destination_cache_entry_key(struct hash_entry *m) {
    return &hash_table_entry(m, struct destination_cache_entry)->destination_path;
}

struct destination_cache_entry *net_bump_destination_cache_entry(struct destination_cache_entry *entry) {
    atomic_fetch_add(&entry->ref_count, 1);
    return entry;
}

void net_drop_destination_cache_entry(struct destination_cache_entry *entry) {
    if (atomic_fetch_sub(&entry->ref_count, 1) == 1) {
        if (entry->next_hop) {
            net_drop_neighbor_cache_entry(entry->next_hop);
        }
        free(entry);
    }
}

static struct neighbor_cache_entry *next_hop_for(struct network_interface *interface, struct ip_v4_address dest_address) {
    if (net_ip_v4_equals(net_ip_v4_mask(interface->address, interface->mask), net_ip_v4_mask(dest_address, interface->mask))) {
        return net_lookup_neighbor(interface, dest_address);
    }

    return net_lookup_neighbor(interface, interface->default_gateway);
}

struct destination_cache_entry *net_lookup_destination(struct network_interface *interface, struct ip_v4_address dest_address) {
    if (interface->type == NETWORK_INTERFACE_LOOPBACK) {
        return net_bump_destination_cache_entry(&loopback_route);
    }

    struct destination_path path = { .local_ip_address = interface->address, .dest_ip_address = dest_address };
    struct destination_cache_entry *entry = hash_get_entry(destination_cache, &path, struct destination_cache_entry);
    if (entry) {
        return net_bump_destination_cache_entry(entry);
    }

    entry = malloc(sizeof(struct destination_cache_entry));
    entry->ref_count = 1;
    entry->destination_path = path;
    entry->next_hop = next_hop_for(interface, dest_address);
    entry->next_packet_id = get_random_bytes() & 0xFFFF;
    hash_put(destination_cache, &entry->hash);
#ifdef DESTINATION_CACHE_DEBUG
    debug_log("Added destination cache entry: [ %d.%d.%d.%d, %d.%d.%d.%d, %d.%d.%d.%d ]\n",
              entry->destination_path.local_ip_address.addr[0], entry->destination_path.local_ip_address.addr[1],
              entry->destination_path.local_ip_address.addr[2], entry->destination_path.local_ip_address.addr[3],
              entry->destination_path.dest_ip_address.addr[0], entry->destination_path.dest_ip_address.addr[1],
              entry->destination_path.dest_ip_address.addr[2], entry->destination_path.dest_ip_address.addr[3],
              entry->next_hop->key.ip_v4_address.addr[0], entry->next_hop->key.ip_v4_address.addr[1],
              entry->next_hop->key.ip_v4_address.addr[2], entry->next_hop->key.ip_v4_address.addr[3]);
#endif /* DESTINATION_CACHE_DEBUG */
    return net_bump_destination_cache_entry(entry);
}

void __net_remove_destination(struct destination_cache_entry *entry) {
#ifdef DESTINATION_CACHE_DEBUG
    debug_log("Removed destination cache entry: [ %d.%d.%d.%d, %d.%d.%d.%d, %d.%d.%d.%d ]\n",
              entry->destination_path.local_ip_address.addr[0], entry->destination_path.local_ip_address.addr[1],
              entry->destination_path.local_ip_address.addr[2], entry->destination_path.local_ip_address.addr[3],
              entry->destination_path.dest_ip_address.addr[0], entry->destination_path.dest_ip_address.addr[1],
              entry->destination_path.dest_ip_address.addr[2], entry->destination_path.dest_ip_address.addr[3],
              entry->next_hop->key.ip_v4_address.addr[0], entry->next_hop->key.ip_v4_address.addr[1],
              entry->next_hop->key.ip_v4_address.addr[2], entry->next_hop->key.ip_v4_address.addr[3]);
#endif /* DESTINATION_CACHE_DEBUG */
    __hash_del(destination_cache, &entry->destination_path);
    net_drop_destination_cache_entry(entry);
}

void net_remove_destination(struct destination_cache_entry *entry) {
    spin_lock(&destination_cache->lock);
    __net_remove_destination(entry);
    spin_unlock(&destination_cache->lock);
}

struct hash_map *net_destination_cache(void) {
    return destination_cache;
}

void init_destination_cache(void) {
    destination_cache = hash_create_hash_map(&destination_cache_entry_hash, &destination_cache_entry_equals, &destination_cache_entry_key);
}
