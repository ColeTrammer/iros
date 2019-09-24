#include <stddef.h>
#include <stdlib.h>

#include <kernel/fs/inode_store.h>
#include <kernel/fs/inode.h>
#include <kernel/hal/output.h>
#include <kernel/util/spinlock.h>

static spinlock_t hash_lock = SPINLOCK_INITIALIZER;
static spinlock_t inode_count_lock = SPINLOCK_INITIALIZER;

/* Should be atomic instead of being locked */
static ino_t inode_count = 0;

static size_t hash(ino_t i) {
	return i % INODE_STORE_HASH_SIZE;
}

static struct hash_entry *hash_table[INODE_STORE_HASH_SIZE];

struct inode *fs_inode_get(ino_t id) {
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

	spin_lock(&hash_lock);

	struct hash_entry **entry = hash_table + i;
	while (*entry != NULL) {
		if ((*entry)->id == inode->index) {
			debug_log("Inode Store put on existing entry: %lld\n", inode->index);
			(*entry)->inode = inode;

			spin_unlock(&hash_lock);
			return;
		}
		
		entry = &(*entry)->next; 
	}
	
	*entry = malloc(sizeof(struct hash_entry));
	(*entry)->id = inode->index;
	(*entry)->next = NULL;
	(*entry)->inode = inode;

	spin_unlock(&hash_lock);
}

void fs_inode_set(struct inode *inode) {
	size_t i = hash(inode->index);

	spin_lock(&hash_lock);

	struct hash_entry *entry = hash_table[i];
	while (entry != NULL) {
		if (entry->id == inode->index) {
			entry->inode = inode;

			spin_unlock(&hash_lock);
			return;
		}

		entry = entry->next;
	}

	spin_unlock(&hash_lock);

	debug_log("Inode Store tried to set non existent entry: %lld\n", inode->index);
}

void fs_inode_del(ino_t id) {
	size_t i = hash(id);

	spin_lock(&hash_lock);

	struct hash_entry **entry = hash_table + i;
	while (*entry != NULL) {
		if ((*entry)->id == id) {
			*entry = (*entry)->next;
			
			spin_unlock(&hash_lock);
			return;
		}

		entry = &(*entry)->next;
	}

	spin_unlock(&hash_lock);

	debug_log("Inode Store tried to delete non existent entry: %lld\n", id);
}

void fs_inode_free_hash_table() {
	spin_lock(&hash_lock);

	for (size_t i = 0; i < INODE_STORE_HASH_SIZE; i++) {
		struct hash_entry *entry = hash_table[i];
		while (entry != NULL) {
			struct hash_entry *temp = entry->next;
			free(entry);
			entry = temp;
		}
	}

	spin_unlock(&hash_lock);
}

ino_t fs_get_next_inode_id() {
	spin_lock(&inode_count_lock);
	ino_t id = inode_count++;
	spin_unlock(&inode_count_lock);
	return id;
}