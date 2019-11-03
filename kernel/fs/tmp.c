#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/param.h>
#include <errno.h>
#include <dirent.h>

#include <kernel/fs/file.h>
#include <kernel/fs/inode.h>
#include <kernel/fs/inode_store.h>
#include <kernel/fs/tmp.h>
#include <kernel/fs/file_system.h>
#include <kernel/fs/vfs.h>
#include <kernel/fs/super_block.h>
#include <kernel/mem/vm_region.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/hal/output.h>
#include <kernel/util/spinlock.h>

static struct file_system fs;
static struct super_block super_block;

static spinlock_t inode_count_lock = SPINLOCK_INITIALIZER;
static ino_t inode_counter = 1;

static struct file_system fs = {
    "tmpfs", 0, &tmp_mount, NULL, NULL
};

// static struct inode_operations tmp_i_op = {
//     NULL, &tmp_lookup, &tmp_open, &tmp_stat, NULL, NULL, NULL, NULL, NULL
// };

static struct inode_operations tmp_dir_i_op = {
    NULL, &tmp_lookup, NULL, &tmp_stat, NULL, NULL, NULL, NULL, NULL
};

// static struct file_operations tmp_f_op = {
//     NULL, &tmp_read, NULL, NULL
// };

// static struct file_operations tmp_dir_f_op = {
//     NULL, NULL, NULL, NULL
// };

struct tnode *tmp_lookup(struct inode *inode, const char *name) {
    /* Assumes we were called on root inode */
    assert(inode->flags & FS_DIR);
    assert(inode->tnode_list != NULL);
    assert(name != NULL);

    struct tnode_list *list = inode->tnode_list;
    while (list != NULL) {
        assert(list->tnode != NULL);
        assert(list->tnode->name != NULL);
        if (strcmp(list->tnode->name, name) == 0) {
            return list->tnode;
        }
        list = list->next;
    }

    return NULL;
}

struct file *tmp_open(struct inode *inode, int flags, int *error) {
    (void) inode;
    (void) flags;

    *error = ENOENT;
    return NULL;
}

ssize_t tmp_read(struct file *file, void *buffer, size_t _len) {
    (void) file;
    (void) buffer;
    (void) _len;

    return 0;
}

int tmp_stat(struct inode *inode, struct stat *stat_struct) {
    stat_struct->st_size = inode->size;
    stat_struct->st_blocks = 1;
    stat_struct->st_blksize = stat_struct->st_size;
    stat_struct->st_ino = inode->index;
    stat_struct->st_dev = inode->device;
    stat_struct->st_mode = inode->mode;
    stat_struct->st_rdev = 0;
    return 0;
}

struct tnode *tmp_mount(struct file_system *current_fs, char *device_path) {
assert(current_fs != NULL);
    assert(strlen(device_path) == 0);

    struct tnode *t_root = calloc(1, sizeof(struct tnode));
    struct inode *root = calloc(1, sizeof(struct inode));

    t_root->inode = root;

    root->device = 2;
    root->flags = FS_DIR;
    root->i_op = &tmp_dir_i_op;
    spin_lock(&inode_count_lock);
    root->index = inode_counter++;
    spin_unlock(&inode_count_lock);
    init_spinlock(&root->lock);
    root->mode = S_IFDIR | 0777;
    root->mounts = NULL;
    root->private_data = NULL;
    root->size = 0;
    root->super_block = &super_block;
    root->tnode_list = NULL;

    super_block.device = root->device;
    super_block.op = NULL;
    super_block.root = t_root;

    current_fs->super_block = &super_block;

    return t_root;
}

void init_tmpfs() {
    load_fs(&fs);
}