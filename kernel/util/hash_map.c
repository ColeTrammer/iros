#include <stddef.h>
#include <stdlib.h>

#include <kernel/hal/output.h>
#include <kernel/util/hash_map.h>
#include <kernel/util/spinlock.h>

#define HASH_MAP_RESIZE_DEBUG

static void __hash_resize_if_needed(struct hash_map *map) {
    if (map->size > 4 * map->num_buckets) {
        size_t new_num_buckets = map->num_buckets * 16;
#ifdef HASH_MAP_RESIZE_DEBUG
        debug_log("resizing hash_map: [ %p, %lu, %lu ]\n", map, map->num_buckets, new_num_buckets);
#endif /* HASH_MAP_RESIZE_DEBUG */

        struct hash_entry **new_buckets = calloc(new_num_buckets, sizeof(struct hash_entry *));

        for (size_t i = 0; i < map->num_buckets; i++) {
            struct hash_entry *entry = map->entries[i];
            while (entry != NULL) {
                struct hash_entry *next = entry->next;

                size_t new_hash = map->hash(map->key(entry), new_num_buckets);
                entry->next = new_buckets[new_hash];
                new_buckets[new_hash] = entry;

                entry = next;
            }
        }

        free(map->entries);
        map->entries = new_buckets;
        map->num_buckets = new_num_buckets;
    }
}

struct hash_map *hash_create_hash_map_with_size(unsigned int (*hash)(void *ptr, int hash_size), int (*equals)(void *ptr, void *id),
                                                void *(*key)(struct hash_entry *ptr), size_t num_buckets) {
    struct hash_map *map = malloc(sizeof(struct hash_map));
    map->hash = hash;
    map->equals = equals;
    map->key = key;
    init_spinlock(&map->lock);
    map->num_buckets = num_buckets;
    map->size = 0;
    map->entries = calloc(map->num_buckets, sizeof(struct hash_entry *));
    return map;
}

struct hash_map *hash_create_hash_map(unsigned int (*hash)(void *ptr, int hash_size), int (*equals)(void *ptr, void *id),
                                      void *(*key)(struct hash_entry *ptr)) {
    return hash_create_hash_map_with_size(hash, equals, key, HASH_DEFAULT_NUM_BUCKETS);
}

struct hash_entry *hash_get(struct hash_map *map, void *key) {
    return hash_get_or_else_do(map, key, NULL, NULL);
}

struct hash_entry *hash_get_at_index(struct hash_map *map, size_t index) {
    if (index >= hash_size(map)) {
        return NULL;
    }

    size_t i = 0;
    for (size_t b = 0; b < map->num_buckets; b++) {
        struct hash_entry *entry = map->entries[b];
        while (entry != NULL) {
            if (i++ == index) {
                return entry;
            }
            entry = entry->next;
        }
    }

    return NULL;
}

struct hash_entry *hash_get_or_else_do(struct hash_map *map, void *key, void (*f)(void *), void *arg) {
    spin_lock(&map->lock);
    size_t i = map->hash(key, map->num_buckets);

    struct hash_entry *entry = map->entries[i];
    while (entry != NULL) {
        assert(entry);
        if (map->equals(map->key(entry), key)) {
            spin_unlock(&map->lock);
            return entry;
        }

        entry = entry->next;
    }

    if (f != NULL) {
        f(arg);
    }

    spin_unlock(&map->lock);
    return NULL;
}

struct hash_entry *hash_put_if_not_present(struct hash_map *map, void *key, struct hash_entry *(*make_data)(void *key)) {
    spin_lock(&map->lock);
    size_t i = map->hash(key, map->num_buckets);

    struct hash_entry **entry = &map->entries[i];
    while (*entry != NULL) {
        void *key_iter = map->key(*entry);
        if (map->equals(key_iter, key)) {
            spin_unlock(&map->lock);
            return *entry;
        }

        entry = &(*entry)->next;
    }

    *entry = make_data(key);
    (*entry)->next = NULL;
    map->size++;
    __hash_resize_if_needed(map);

    spin_unlock(&map->lock);
    return *entry;
}

void hash_put(struct hash_map *map, struct hash_entry *data) {
    spin_lock(&map->lock);
    size_t i = map->hash(map->key(data), map->num_buckets);

    struct hash_entry **entry = &map->entries[i];
    while (*entry != NULL) {
        void *key_iter = map->key(*entry);
        if (map->equals(key_iter, map->key(data))) {
            data->next = (*entry)->next;
            *entry = data;
            spin_unlock(&map->lock);
            debug_log("HASH PUT DUPLICATE\n");
            return;
        }

        entry = &(*entry)->next;
    }

    *entry = data;
    (*entry)->next = NULL;
    map->size++;
    __hash_resize_if_needed(map);

    spin_unlock(&map->lock);
}

struct hash_entry *__hash_del(struct hash_map *map, void *key) {
    size_t i = map->hash(key, map->num_buckets);

    struct hash_entry **entry = &map->entries[i];
    while (*entry != NULL) {
        void *key_iter = map->key(*entry);
        if (map->equals(key_iter, key)) {
            struct hash_entry *data_removed = *entry;
            struct hash_entry *next = (*entry)->next;
            *entry = next;
            map->size--;

            return data_removed;
        }

        entry = &(*entry)->next;
    }

    return NULL;
}

struct hash_entry *hash_del(struct hash_map *map, void *key) {
    spin_lock(&map->lock);
    struct hash_entry *ret = __hash_del(map, key);
    spin_unlock(&map->lock);
    return ret;
}

void hash_free_hash_map(struct hash_map *map) {
    free(map->entries);
    free(map);
}

void hash_for_each(struct hash_map *map, void (*f)(struct hash_entry *o, void *d), void *d) {
    spin_lock(&map->lock);

    for (size_t i = 0; i < map->num_buckets; i++) {
        struct hash_entry *entry = map->entries[i];
        while (entry != NULL) {
            struct hash_entry *next = entry->next;
            f(entry, d);
            entry = next;
        }
    }

    spin_unlock(&map->lock);
}
