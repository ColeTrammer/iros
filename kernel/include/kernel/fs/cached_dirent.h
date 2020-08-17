#ifndef _KERNEL_FS_CACHED_DIRENT_H
#define _KERNEL_FS_CACHED_DIRENT_H 1

#include <stddef.h>
#include <sys/types.h>

#include <kernel/util/hash_map.h>

struct cached_dirent {
    char *name;
    struct inode *inode;
    struct hash_entry hash;
};

struct hash_map *fs_create_dirent_cache(void);
void fs_destroy_dirent_cache(struct hash_map *map);
struct inode *fs_lookup_in_cache(struct hash_map *map, const char *name);
void fs_put_dirent_cache(struct hash_map *map, struct inode *inode, const char *name, size_t length);
void fs_del_dirent_cache(struct hash_map *map, const char *name);

struct cached_dirent *fs_lookup_in_cache_with_index(struct hash_map *map, off_t position);
int fs_get_dirent_cache_size(struct hash_map *map);
void fs_dirent_cache_for_each(struct hash_map *map, void (*f)(struct hash_entry *o, void *d), void *d);

#endif /* _KERNEL_FS_CACHED_DIRENT_H */
