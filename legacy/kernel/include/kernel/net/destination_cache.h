#ifndef _KERNEL_NET_DESTINATION_CACHE_H
#define _KERNEL_NET_DESTINATION_CACHE_H 1

#include <kernel/net/ip_address.h>
#include <kernel/util/hash_map.h>

struct neighbor_cache_entry;
struct network_interface;

struct destination_path {
    struct ip_v4_address local_ip_address;
    struct ip_v4_address dest_ip_address;
};

struct destination_cache_entry {
    struct hash_entry hash;
    struct neighbor_cache_entry *next_hop;
    struct destination_path destination_path;
    int ref_count;
    uint16_t next_packet_id;
};

struct destination_cache_entry *net_bump_destination_cache_entry(struct destination_cache_entry *entry);
void net_drop_destination_cache_entry(struct destination_cache_entry *entry);

struct destination_cache_entry *net_lookup_destination(struct network_interface *interface, struct ip_v4_address dest_address);
void __net_remove_destination(struct destination_cache_entry *entry);
void net_remove_destination(struct destination_cache_entry *entry);

struct hash_map *net_destination_cache(void);

#endif /* _KERNEL_NET_DESTINATION_CACHE_H */
