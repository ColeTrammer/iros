#ifndef _KERNEL_FS_MOUNT_H
#define _KERNEL_FS_MOUNT_H 1

#include <kernel/util/list.h>
#include <kernel/util/spinlock.h>

struct file_system;
struct super_block;

struct mount {
    char *name;
    struct file_system *fs;
    struct super_block *super_block;
    struct mount *parent;
    struct list_node list;
    int busy_count;
};

void fs_decrement_mount_busy_count(struct mount *mount);
struct mount *fs_increment_mount_busy_count(struct mount *mount);

struct mount *fs_create_mount(struct super_block *super_block, struct file_system *file_system, struct mount *parent, char *name);
void fs_free_mount(struct mount *mount);

void fs_for_each_mount(void (*cb)(struct mount *mount, void *closure), void *closure);
void fs_register_mount(struct mount *mount);
void fs_unregister_mount(struct mount *mount);

#endif /* _KERNEL_FS_MOUNT_H */
