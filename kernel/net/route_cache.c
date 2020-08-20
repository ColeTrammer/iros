#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/hal/output.h>
#include <kernel/net/interface.h>
#include <kernel/net/route_cache.h>

#define ROUTE_CACHE_DEBUG

static struct hash_map *route_cache;

static struct route_cache_entry loopback_route = {
    .route_path = {
        .local_ip_address = IP_V4_LOOPBACK,
        .dest_ip_address = IP_V4_LOOPBACK,
    },
    .next_hop_address = IP_V4_LOOPBACK,
    .hash = { 0 },
    .ref_count = 1,
};

static unsigned int route_cache_entry_hash(void *_path, int num_buckets) {
    struct route_path *path = _path;
    return (path->local_ip_address.addr[0] + path->local_ip_address.addr[1] + path->local_ip_address.addr[2] +
            path->local_ip_address.addr[3] + path->dest_ip_address.addr[0] + path->dest_ip_address.addr[1] + path->dest_ip_address.addr[2] +
            path->dest_ip_address.addr[3]) %
           num_buckets;
}

static int route_cache_entry_equals(void *i1, void *i2) {
    return memcmp(i1, i2, sizeof(struct route_path)) == 0;
}

static void *route_cache_entry_key(struct hash_entry *m) {
    return &hash_table_entry(m, struct route_cache_entry)->route_path;
}

struct route_cache_entry *net_bump_route_cache_entry(struct route_cache_entry *entry) {
    atomic_fetch_add(&entry->ref_count, 1);
    return entry;
}

void net_drop_route_cache_entry(struct route_cache_entry *entry) {
    if (atomic_fetch_sub(&entry->ref_count, 1) == 1) {
        free(entry);
    }
}

struct ip_v4_address next_hop_for(struct network_interface *interface, struct ip_v4_address dest_address) {
    (void) dest_address;
    // FIXME: return the dest address if it is local according the interface's network mask.
    return interface->default_gateway;
}

struct route_cache_entry *net_find_next_hop_gateway(struct network_interface *interface, struct ip_v4_address dest_address) {
    if (interface->type == NETWORK_INTERFACE_LOOPBACK) {
        return net_bump_route_cache_entry(&loopback_route);
    }

    struct route_path path = { .local_ip_address = interface->address, .dest_ip_address = dest_address };
    struct route_cache_entry *entry = hash_get_entry(route_cache, &path, struct route_cache_entry);
    if (entry) {
        return net_bump_route_cache_entry(entry);
    }

    entry = malloc(sizeof(struct route_cache_entry));
    entry->ref_count = 1;
    entry->route_path = path;
    entry->next_hop_address = next_hop_for(interface, dest_address);
    hash_put(route_cache, &entry->hash);
#ifdef ROUTE_CACHE_DEBUG
    debug_log("Added route cache entry: [ %d.%d.%d.%d, %d.%d.%d.%d, %d.%d.%d.%d ]\n", entry->route_path.local_ip_address.addr[0],
              entry->route_path.local_ip_address.addr[1], entry->route_path.local_ip_address.addr[2],
              entry->route_path.local_ip_address.addr[3], entry->route_path.dest_ip_address.addr[0],
              entry->route_path.dest_ip_address.addr[1], entry->route_path.dest_ip_address.addr[2],
              entry->route_path.dest_ip_address.addr[3], entry->next_hop_address.addr[0], entry->next_hop_address.addr[1],
              entry->next_hop_address.addr[2], entry->next_hop_address.addr[3]);
#endif /* ROUTE_CACHE_DEBUG */
    return net_bump_route_cache_entry(entry);
}

void init_route_cache(void) {
    route_cache = hash_create_hash_map(&route_cache_entry_hash, &route_cache_entry_equals, &route_cache_entry_key);
}
