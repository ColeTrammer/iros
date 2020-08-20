#ifndef _KERNEL_NET_ROUTE_CACHE_H
#define _KERNEL_NET_ROUTE_CACHE_H 1

#include <kernel/net/ip_address.h>
#include <kernel/util/hash_map.h>

struct network_interface;

struct route_path {
    struct ip_v4_address local_ip_address;
    struct ip_v4_address dest_ip_address;
};

struct route_cache_entry {
    struct route_path route_path;
    struct ip_v4_address next_hop_address;
    struct hash_entry hash;
    int ref_count;
};

struct route_cache_entry *net_bump_route_cache_entry(struct route_cache_entry *entry);
void net_drop_route_cache_entry(struct route_cache_entry *entry);

struct route_cache_entry *net_find_next_hop_gateway(struct network_interface *interface, struct ip_v4_address dest_address);

void init_route_cache(void);

#endif /* _KERNEL_NET_ROUTE_CACHE_H */
