#include <assert.h>
#include <string.h>

#include <stdlib.h>
#include <kernel/fs/cached_dirent.h>
#include <kernel/fs/vfs.h>
#include <kernel/hal/output.h>
#include <kernel/util/hash_map.h>

// #define DIRENT_CACHE_DEBUG

static unsigned int cached_dirent_hash(void *d, int num_buckets) {
    char *s = d;
    unsigned int v = 0;
    for (int i = 0; s[i] != '\0'; i++) {
        v += ~s[i];
        v ^= s[i];
    }

    return v % num_buckets;
}

static int cached_dirent_equals(void *a, void *b) {
    return strcmp(a, b) == 0;
}

static void *cached_dirent_key(void *d) {
    return ((struct cached_dirent *) d)->name;
}

struct hash_map *fs_create_dirent_cache(void) {
    return hash_create_hash_map(cached_dirent_hash, cached_dirent_equals, cached_dirent_key);
}

struct inode *fs_lookup_in_cache(struct hash_map *map, const char *name) {
    assert(map);
    struct cached_dirent *result = hash_get(map, (void *) name);
    if (result) {
        return result->inode;
    }

    return NULL;
}

void fs_put_dirent_cache(struct hash_map *map, struct inode *inode, const char *name, size_t length) {
    assert(map);
    struct cached_dirent *to_add = malloc(sizeof(struct cached_dirent));
    to_add->inode = inode;
    bump_inode_reference(inode);
    to_add->name = strndup(name, length);

#ifdef DIRENT_CACHE_DEBUG
    debug_log("Adding: [ %s ]\n", to_add->name);
#endif /* DIRENT_CACHE_DEBUG */

    assert(!fs_lookup_in_cache(map, to_add->name));
    hash_put(map, to_add);
}

static void do_destroy_dirent(void *_dirent, void *_ __attribute__((unused))) {
    struct cached_dirent *dirent = _dirent;
    struct inode *inode = dirent->inode;

#ifdef DIRENT_CACHE_DEBUG
    debug_log("Destroying: [ %s, %lu, %llu ]\n", dirent->name, inode->device, inode->index);
#endif /* DIRENT_CACHE_DEBUG */

    free(dirent->name);
    free(dirent);
    drop_inode_reference(inode);
}

void fs_del_dirent_cache(struct hash_map *map, const char *name) {
    struct cached_dirent *dirent = hash_del(map, (void *) name);
    if (dirent) {
        do_destroy_dirent(dirent, NULL);
    }
}

void fs_destroy_dirent_cache(struct hash_map *map) {
    hash_for_each(map, do_destroy_dirent, NULL);
    hash_free_hash_map(map);
}

struct cached_dirent *fs_lookup_in_cache_with_index(struct hash_map *map, off_t position) {
    assert(map);
    return hash_get_at_index(map, position);
}

int fs_get_dirent_cache_size(struct hash_map *map) {
    assert(map);
    return hash_size(map);
}

void fs_dirent_cache_for_each(struct hash_map *map, void (*f)(void *o, void *d), void *d) {
    assert(map);
    return hash_for_each(map, f, d);
}
