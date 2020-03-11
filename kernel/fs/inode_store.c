#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#include <kernel/fs/inode.h>
#include <kernel/fs/inode_store.h>
#include <kernel/hal/output.h>
#include <kernel/util/hash_map.h>
#include <kernel/util/spinlock.h>

static struct hash_map *dev_map;

HASH_DEFINE_FUNCTIONS(gen, struct inode, ino_t, index)
HASH_DEFINE_FUNCTIONS(dev, struct inode_store, dev_t, device)

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
    debug_log("dev: [ %lu, %llu ]\n", inode->device, inode->index);

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