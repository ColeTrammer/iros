#ifndef _KERNEL_UTIL_HASH_MAP_H
#define _KERNEL_UTIL_HASH_MAP_H 1

#include <kernel/util/spinlock.h>

#define HASH_DEFAULT_NUM_BUCKETS 25

struct hash_entry {
    void *key;
    void *data;
    struct hash_entry *next;
};

struct hash_map {
    int (*hash)(void *ptr, int hash_size);
    int (*equals)(void *ptr, void *id);
    void *(*key)(void *ptr);
    spinlock_t lock;
    size_t num_buckets;
    size_t size;
    struct hash_entry **entries;
};

static inline __attribute__((always_inline)) size_t hash_size(struct hash_map *map) {
    return map->size;
}

struct hash_map *hash_create_hash_map_with_size(int (*hash)(void *ptr, int hash_size), int (*equals)(void *ptr, void *id),
                                                void *(*key)(void *ptr), size_t num_buckets);
struct hash_map *hash_create_hash_map(int (*hash)(void *ptr, int hash_size), int (*equals)(void *ptr, void *id), void *(*key)(void *ptr));
void *hash_get(struct hash_map *map, void *key);
void *hash_get_or_else_do(struct hash_map *map, void *key, void (*f)(void *), void *arg);
void *hash_put_if_not_present(struct hash_map *map, void *key, void *(*make_data)(void *key));
void hash_put(struct hash_map *map, void *ptr);
void hash_set(struct hash_map *map, void *ptr);
void *hash_del(struct hash_map *map, void *key);
void hash_free_hash_map(struct hash_map *map);

void hash_for_each(struct hash_map *map, void (*f)(void *o, void *d), void *d);

#endif /* _KERNEL_UTIL_HASH_MAP_H */