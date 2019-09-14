#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <kernel/fs/file.h>
#include <kernel/fs/inode.h>
#include <kernel/fs/inode_store.h>
#include <kernel/fs/dev.h>
#include <kernel/fs/file_system.h>
#include <kernel/fs/vfs.h>
#include <kernel/fs/super_block.h>
#include <kernel/mem/vm_region.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/hal/output.h>
#include <kernel/util/spinlock.h>

static struct file_system fs;
static struct super_block super_block;

static struct file_system fs = {
    "dev", 0, &dev_mount, NULL, NULL
};

static struct inode_operations dev_i_op = {
    &dev_lookup, &dev_open
};

static struct file_operations dev_f_op = {
    &dev_close, &dev_read, &dev_write
};

struct tnode *dev_lookup(struct inode *inode, const char *name) {

}

struct file *dev_open(struct inode *inode) {

}

void dev_close(struct file *file) {
    free(file);
}

void dev_read(struct file *file, void *buffer, size_t len) {

}

void dev_write(struct file *file, const void *buffer, size_t len) {

}

struct tnode *dev_mount(struct file_system *current_fs) {
    assert(current_fs != NULL);

    struct tnode *t_root = calloc(1, sizeof(struct tnode));
    struct inode *root = calloc(1, sizeof(struct inode));

    t_root->name = "/";
    t_root->inode = root;

    root->device = 0;
    root->flags = FS_DIR;
    root->i_op = &dev_i_op;
    root->index = fs_get_next_inode_id();
    init_spinlock(&root->lock);
    root->mode = 0;
    root->mounts = NULL;
    root->private_data = NULL;
    root->size = 0;
    root->super_block = &super_block;
    root->tnode_list = NULL;

    super_block.device = 0;
    super_block.op = NULL;
    super_block.root = t_root;

    current_fs->super_block = &super_block;

    return t_root;
}

void init_dev() {
    load_fs(&fs);
}