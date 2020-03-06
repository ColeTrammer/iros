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

                size_t new_hash = map->hash(entry->key, new_num_buckets);
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

struct hash_map *hash_create_hash_map_with_size(int (*hash)(void *ptr, int hash_size), int (*equals)(void *ptr, void *id),
                                                void *(*key)(void *ptr), size_t num_buckets) {
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

struct hash_map *hash_create_hash_map(int (*hash)(void *ptr, int hash_size), int (*equals)(void *ptr, void *id), void *(*key)(void *ptr)) {
    return hash_create_hash_map_with_size(hash, equals, key, HASH_DEFAULT_NUM_BUCKETS);
}

void *hash_get(struct hash_map *map, void *key) {
    return hash_get_or_else_do(map, key, NULL, NULL);
}

void *hash_get_or_else_do(struct hash_map *map, void *key, void (*f)(void *), void *arg) {
    spin_lock(&map->lock);
    size_t i = map->hash(key, map->num_buckets);

    struct hash_entry *entry = map->entries[i];
    while (entry != NULL) {
        assert(entry);
        assert(entry->key);
        if (map->equals(entry->key, key)) {
            spin_unlock(&map->lock);
            return entry->data;
        }

        entry = entry->next;
    }

    if (f != NULL) {
        f(arg);
    }

    spin_unlock(&map->lock);
    return NULL;
}

void *hash_put_if_not_present(struct hash_map *map, void *key, void *(*make_data)(void *key)) {
    spin_lock(&map->lock);
    size_t i = map->hash(key, map->num_buckets);

    struct hash_entry **entry = &map->entries[i];
    while (*entry != NULL) {
        if (map->equals((*entry)->key, key)) {
            spin_unlock(&map->lock);
            return (*entry)->data;
        }

        entry = &(*entry)->next;
    }

    *entry = calloc(1, sizeof(struct hash_entry));
    (*entry)->next = NULL;
    (*entry)->data = make_data(key);
    (*entry)->key = map->key((*entry)->data);
    map->size++;
    __hash_resize_if_needed(map);

    spin_unlock(&map->lock);
    return (*entry)->data;
}

void hash_put(struct hash_map *map, void *data) {
    spin_lock(&map->lock);
    size_t i = map->hash(map->key(data), map->num_buckets);

    struct hash_entry **entry = &map->entries[i];
    while (*entry != NULL) {
        if (map->equals((*entry)->key, map->key(data))) {
            (*entry)->data = data;

            spin_unlock(&map->lock);
            return;
        }

        entry = &(*entry)->next;
    }

    *entry = calloc(1, sizeof(struct hash_entry));
    (*entry)->key = map->key(data);
    assert((*entry)->key);
    (*entry)->next = NULL;
    (*entry)->data = data;
    map->size++;
    __hash_resize_if_needed(map);

    spin_unlock(&map->lock);
}

void hash_set(struct hash_map *map, void *data) {
    spin_lock(&map->lock);
    size_t i = map->hash(map->key(data), map->num_buckets);

    struct hash_entry *entry = map->entries[i];
    while (entry != NULL) {
        if (map->equals(entry->key, map->key(data))) {
            entry->data = data;

            spin_unlock(&map->lock);
            return;
        }

        entry = entry->next;
    }

    spin_unlock(&map->lock);

    debug_log("Hash Map tried to set non existent entry\n");
}

void *hash_del(struct hash_map *map, void *key) {
    spin_lock(&map->lock);
    size_t i = map->hash(key, map->num_buckets);

    struct hash_entry **entry = &map->entries[i];
    while (*entry != NULL) {
        if (map->equals((*entry)->key, key)) {
            void *data_removed = (*entry)->data;
            struct hash_entry *next = (*entry)->next;
            free(*entry);
            *entry = next;
            map->size--;

            spin_unlock(&map->lock);
            return data_removed;
        }

        entry = &(*entry)->next;
    }

    spin_unlock(&map->lock);
    return NULL;
}

void hash_free_hash_map(struct hash_map *map) {
    spin_lock(&map->lock);

    for (size_t i = 0; i < map->num_buckets; i++) {
        struct hash_entry *entry = map->entries[i];
        while (entry != NULL) {
            struct hash_entry *temp = entry->next;
            free(entry);
            entry = temp;
        }
    }

    spin_unlock(&map->lock);
}

void hash_for_each(struct hash_map *map, void (*f)(void *o, void *d), void *d) {
    spin_lock(&map->lock);

    for (size_t i = 0; i < map->num_buckets; i++) {
        struct hash_entry *entry = map->entries[i];
        while (entry != NULL) {
            f(entry->data, d);
            entry = entry->next;
        }
    }

    spin_unlock(&map->lock);
}