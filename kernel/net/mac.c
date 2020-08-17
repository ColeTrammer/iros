#include <stdlib.h>
#include <string.h>

#include <kernel/net/ip_address.h>
#include <kernel/net/mac.h>
#include <kernel/util/hash_map.h>

static struct hash_map *map;

static unsigned int hash(void *ip_addr, int num_buckets) {
    struct ip_v4_address *addr = ip_addr;

    int sum = 0;
    for (size_t i = 0; i < sizeof(struct ip_v4_address); i++) {
        sum += addr->addr[i];
    }

    return sum % num_buckets;
}

static int equals(void *i1, void *i2) {
    return memcmp(i1, i2, sizeof(struct ip_v4_address)) == 0;
}

static void *key(struct hash_entry *mapping) {
    return &hash_table_entry(mapping, struct ip_v4_to_mac_mapping)->ip;
}

struct hash_map *net_ip_v4_to_mac_table(void) {
    return map;
}

struct ip_v4_to_mac_mapping *net_get_mac_from_ip_v4(struct ip_v4_address address) {
    return hash_get_entry(map, &address, struct ip_v4_to_mac_mapping);
}

void net_create_ip_v4_to_mac_mapping(struct ip_v4_address ip_address, struct mac_address mac_address) {
    struct ip_v4_to_mac_mapping *m = malloc(sizeof(struct ip_v4_to_mac_mapping));
    m->ip = ip_address;
    m->mac = mac_address;

    hash_put(map, &m->hash);
}

void init_mac() {
    map = hash_create_hash_map(hash, equals, key);
    assert(map);
}
