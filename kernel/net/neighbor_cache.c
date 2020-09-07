#include <assert.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/hal/output.h>
#include <kernel/net/neighbor_cache.h>
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
        hash_del(neighbor_cache, &neighbor->ip_v4_address);
        if (neighbor->update_timer) {
            time_cancel_kernel_callback(neighbor->update_timer);
        }
        assert(list_is_empty(&neighbor->queued_packets));
        free(neighbor);
    }
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
    neighbor->state = NS_REACHABLE;
    neighbor->link_layer_address = confirmed_address;

#ifdef NEIGHBOR_CACHE_DEBUG
    debug_log("Confirmed link layer address of neighbor: [ %d.%d.%d.%d, %02x:%02x:%02x:%02x:%02x:%02x ]\n", neighbor->ip_v4_address.addr[0],
              neighbor->ip_v4_address.addr[1], neighbor->ip_v4_address.addr[2], neighbor->ip_v4_address.addr[3],
              neighbor->link_layer_address.addr[0], neighbor->link_layer_address.addr[1], neighbor->link_layer_address.addr[2],
              neighbor->link_layer_address.addr[3], neighbor->link_layer_address.addr[4], neighbor->link_layer_address.addr[5]);
#endif /* NEIGHBOR_CACHE_DEBUG */
    spin_unlock(&neighbor->lock);
}

struct hash_map *net_neighbor_cache(void) {
    return neighbor_cache;
}

void init_neighbor_cache(void) {
    neighbor_cache = hash_create_hash_map(neighbor_cache_entry_hash, neighbor_cache_entry_equals, neighbor_cache_entry_key);
}
