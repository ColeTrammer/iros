#include <stdatomic.h>
#include <stdlib.h>

#include <kernel/fs/mount.h>

static struct list_node mounts_list = INIT_LIST(mounts_list);
static spinlock_t mounts_list_lock = SPINLOCK_INITIALIZER;

void fs_for_each_mount(void (*cb)(struct mount *mount, void *closure), void *closure) {
    spin_lock(&mounts_list_lock);
    list_for_each_entry_safe(&mounts_list, mount, struct mount, list) { cb(mount, closure); }
    spin_unlock(&mounts_list_lock);
}

void fs_register_mount(struct mount *mount) {
    spin_lock(&mounts_list_lock);
    list_append(&mounts_list, &mount->list);
    spin_unlock(&mounts_list_lock);
}

void fs_unregister_mount(struct mount *mount) {
    spin_lock(&mounts_list_lock);
    list_remove(&mount->list);
    spin_unlock(&mounts_list_lock);
}

void fs_decrement_mount_busy_count(struct mount *mount) {
    atomic_fetch_sub(&mount->busy_count, 1);
}

struct mount *fs_increment_mount_busy_count(struct mount *mount) {
    atomic_fetch_add(&mount->busy_count, 1);
    return mount;
}

struct mount *fs_create_mount(struct super_block *super_block, struct file_system *file_system, char *name) {
    struct mount *mount = calloc(1, sizeof(*mount));
    mount->name = name;
    mount->fs = file_system;
    mount->super_block = super_block;

    fs_register_mount(mount);

    return mount;
}

void fs_free_mount(struct mount *mount) {
    fs_unregister_mount(mount);
    free(mount->name);
    free(mount);
}
