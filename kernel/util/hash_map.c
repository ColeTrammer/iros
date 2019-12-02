#include <stddef.h>
#include <stdlib.h>

#include <kernel/hal/output.h>
#include <kernel/util/hash_map.h>
#include <kernel/util/spinlock.h>

struct hash_map *hash_create_hash_map(int (*hash)(void *ptr, int hash_size), int (*equals)(void *ptr, void *id), void *(*key)(void *ptr)) {
    struct hash_map *map = calloc(1, sizeof(struct hash_map));
    map->hash = hash;
    map->equals = equals;
    map->key = key;
    init_spinlock(&map->lock);
    return map;
}

void *hash_get(struct hash_map *map, void *key) {
    size_t i = map->hash(key, HASH_DEFAULT_NUM_BUCKETS);

    spin_lock(&map->lock);

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

    spin_unlock(&map->lock);
    return NULL;
}

void hash_put(struct hash_map *map, void *data) {
    size_t i = map->hash(map->key(data), HASH_DEFAULT_NUM_BUCKETS);

    spin_lock(&map->lock);

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

    spin_unlock(&map->lock);
}

void hash_set(struct hash_map *map, void *data) {
    size_t i = map->hash(map->key(data), HASH_DEFAULT_NUM_BUCKETS);

    spin_lock(&map->lock);

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
    size_t i = map->hash(key, HASH_DEFAULT_NUM_BUCKETS);

    spin_lock(&map->lock);

    struct hash_entry **entry = &map->entries[i];
    while (*entry != NULL) {
        if (map->equals((*entry)->key, key)) {
            void *data_removed = (*entry)->data;
            struct hash_entry *next = (*entry)->next;
            free(*entry);
            *entry = next;

            spin_unlock(&map->lock);
            return data_removed;
        }

        entry = &(*entry)->next;
    }

    spin_unlock(&map->lock);

    debug_log("Hash Map tried to delete non existent entry\n");
    return NULL;
}

void hash_free_hash_map(struct hash_map *map) {
    spin_lock(&map->lock);

    for (size_t i = 0; i < HASH_DEFAULT_NUM_BUCKETS; i++) {
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

    for (size_t i = 0; i < HASH_DEFAULT_NUM_BUCKETS; i++) {
        struct hash_entry *entry = map->entries[i];
        while (entry != NULL) {
            f(entry->data, d);
            entry = entry->next;
        }
    }

    spin_unlock(&map->lock);
}