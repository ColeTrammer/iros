#include <stddef.h>
#include <stdlib.h>

#include <kernel/fs/inode_store.h>
#include <kernel/fs/inode.h>
#include <kernel/hal/output.h>
#include <kernel/util/hash_map.h>
#include <kernel/util/spinlock.h>

static spinlock_t inode_count_lock = SPINLOCK_INITIALIZER;

/* Should be atomic instead of being locked */
static ino_t inode_count = 0;

static struct hash_map *dev_map;

static int gen_hash(void *inode_id, int hash_size) {
	return *((ino_t*) inode_id) % hash_size;
}

static int gen_equals(void *inode_id1, void *inode_id2) {
	return *((ino_t*) inode_id1) == *((ino_t*) inode_id2);
}

static void *gen_key(void *inode) {
	return &((struct inode*) inode)->index;
}

void init_fs_inode_store() {
	dev_map = hash_create_hash_map(gen_hash, gen_equals, gen_key);
}

struct inode *fs_inode_get(ino_t id) {
	return hash_get(dev_map, &id);
}

void fs_inode_put(struct inode *inode) {
	hash_put(dev_map, inode);
}

void fs_inode_set(struct inode *inode) {
	hash_set(dev_map, inode);
}

void fs_inode_del(ino_t id) {
	hash_del(dev_map, &id);
}

void fs_inode_free_hash_table() {
	hash_free_hash_map(dev_map);
}

ino_t fs_get_next_inode_id() {
	spin_lock(&inode_count_lock);
	ino_t id = inode_count++;
	spin_unlock(&inode_count_lock);
	return id;
}