#include <stddef.h>
#include <stdlib.h>

#include <kernel/hal/output.h>
#include <kernel/util/hash_map.h>
#include <kernel/util/spinlock.h>

struct hash_map *hash_create_hash_map(int (*hash)(void *ptr), int (*equals)(void *ptr, void *id), void *(*key)(void *ptr)) {
    struct hash_map *map = calloc(1, sizeof(struct hash_map));
    map->hash = hash;
    map->equals = equals;
    map->key = key;
    init_spinlock(&map->lock);
    return map;
}

void *hash_get(struct hash_map *map, void *key) {
	size_t i = map->hash(map->key(key));

	struct hash_entry *entry = map->entries[i];
	while (entry != NULL) {
		if (map->equals(entry->key, key)) {
			return entry->data;
		}

		entry = entry->next;
	}
	
	return NULL;
}

void hash_put(struct hash_map *map, void *data) {
	size_t i = map->hash(map->key(data));

	spin_lock(&map->lock);

	struct hash_entry **entry = &map->entries[i];
	while (*entry != NULL) {
		if (map->equals((*entry)->key, map->key(data))) {
			debug_log("Hash Map put on existing entry\n");
			(*entry)->data = data;

			spin_unlock(&map->lock);
			return;
		}
		
		entry = &(*entry)->next; 
	}
	
	*entry = malloc(sizeof(struct hash_entry));
	(*entry)->key = map->key(data);
	(*entry)->next = NULL;
	(*entry)->data = data;

	spin_unlock(&map->lock);
}

void hash_set(struct hash_map *map, void *data) {
	size_t i = map->hash(map->key(data));

	spin_lock(&map->lock);

	struct hash_entry *entry = map->entries[i];
	while (entry != NULL) {
		if (entry->key == map->key(data)) {
			entry->data = data;

			spin_unlock(&map->lock);
			return;
		}

		entry = entry->next;
	}

	spin_unlock(&map->lock);

	debug_log("Hash Map tried to set non existent entry\n");
}

void hash_del(struct hash_map *map, void *key) {
	size_t i = map->hash(key);

	spin_lock(&map->lock);

	struct hash_entry **entry = &map->entries[i];
	while (*entry != NULL) {
		if (map->equals((*entry)->key, key)) {
			*entry = (*entry)->next;
			
			spin_unlock(&map->lock);
			return;
		}

		entry = &(*entry)->next;
	}

	spin_unlock(&map->lock);

	debug_log("Hash Map tried to delete non existent entry\n");
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