#include <stddef.h>
#include <stdlib.h>

#include <kernel/fs/inode_store.h>
#include <kernel/fs/inode.h>
#include <kernel/hal/output.h>

static size_t hash(inode_id_t i) {
	return i % INODE_STORE_HASH_SIZE;
}

static struct hash_entry *hash_table[INODE_STORE_HASH_SIZE];

struct inode *fs_inode_get(inode_id_t id) {
	size_t i = hash(id);

	struct hash_entry *entry = hash_table[i];
	while (entry != NULL) {
		if (entry->id == id) {
			return entry->inode;
		}

		entry = entry->next;
	}
	
	return NULL;
}

void fs_inode_put(struct inode *inode) {
	size_t i = hash(inode->index);

	struct hash_entry **entry = hash_table + i;
	while (*entry != NULL) {
		if ((*entry)->id == inode->index) {
			debug_log("Inode Store put on existing entry: %ld\n", inode->index);
			(*entry)->inode = inode;
			return;
		}
		
		entry = &(*entry)->next; 
	}
	
	*entry = malloc(sizeof(struct hash_entry));
	(*entry)->id = inode->index;
	(*entry)->next = NULL;
	(*entry)->inode = inode;
}

void fs_inode_set(struct inode *inode) {
	size_t i = hash(inode->index);

	struct hash_entry *entry = hash_table[i];
	while (entry != NULL) {
		if (entry->id == inode->index) {
			entry->inode = inode;
			return;
		}

		entry = entry->next;
	}

	debug_log("Inode Store tried to set non existent entry: %ld\n", inode->index);
}

void fs_inode_del(inode_id_t id) {
	size_t i = hash(id);

	struct hash_entry **entry = hash_table + i;
	while (*entry != NULL) {
		if ((*entry)->id == id) {
			*entry = (*entry)->next;
			return;
		}

		entry = &(*entry)->next;
	}

	debug_log("Inode Store tried to delete non existent entry: %ld\n", id);
}

void fs_inode_free_hash_table() {
	for (size_t i = 0; i < INODE_STORE_HASH_SIZE; i++) {
		struct hash_entry *entry = hash_table[i];
		while (entry != NULL) {
			struct hash_entry *temp = entry->next;
			free(entry);
			entry = temp;
		}
	}
}