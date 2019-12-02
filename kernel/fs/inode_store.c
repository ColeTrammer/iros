#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#include <kernel/fs/inode.h>
#include <kernel/fs/inode_store.h>
#include <kernel/hal/output.h>
#include <kernel/util/hash_map.h>
#include <kernel/util/spinlock.h>

static struct hash_map *dev_map;

static int gen_hash(void *inode_id, int hash_size) {
    return *((ino_t *) inode_id) % hash_size;
}

static int gen_equals(void *inode_id1, void *inode_id2) {
    return *((ino_t *) inode_id1) == *((ino_t *) inode_id2);
}

static void *gen_key(void *inode) {
    return &((struct inode *) inode)->index;
}

static int dev_hash(void *dev_num, int hash_size) {
    return *((dev_t *) dev_num) % hash_size;
}

static int dev_equals(void *dev1, void *dev2) {
    return *((dev_t *) dev1) == *((dev_t *) dev2);
}

static void *dev_key(void *store) {
    return &((struct inode_store *) store)->device;
}

void init_fs_inode_store() {
    dev_map = hash_create_hash_map(dev_hash, dev_equals, dev_key);
}

void fs_inode_create_store(dev_t dev) {
    struct inode_store *store = malloc(sizeof(struct inode_store));
    store->device = dev;
    store->map = hash_create_hash_map(gen_hash, gen_equals, gen_key);

    hash_put(dev_map, store);
}

struct inode *fs_inode_get(dev_t dev, ino_t id) {
    struct inode_store *store = hash_get(dev_map, &dev);
    if (store == NULL) {
        return NULL;
    }

    return hash_get(store->map, &id);
}

void fs_inode_put(struct inode *inode) {
    struct inode_store *store = hash_get(dev_map, &inode->device);
    assert(store);

    hash_put(store->map, inode);
}

void fs_inode_set(struct inode *inode) {
    struct inode_store *store = hash_get(dev_map, &inode->device);
    assert(store);

    hash_set(store->map, inode);
}

void fs_inode_del(dev_t dev, ino_t id) {
    struct inode_store *store = hash_get(dev_map, &dev);
    assert(store);

    hash_del(store->map, &id);
}

void fs_inode_free_store(dev_t dev) {
    struct inode_store *store = hash_get(dev_map, &dev);

    hash_free_hash_map(store->map);
    free(store);
}