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
#include <kernel/fs/ext2.h>
#include <kernel/fs/file_system.h>
#include <kernel/fs/vfs.h>
#include <kernel/fs/super_block.h>
#include <kernel/mem/vm_region.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/hal/output.h>
#include <kernel/util/spinlock.h>
#include <kernel/hal/x86_64/drivers/serial.h>

static struct file_system fs = {
    "ext2", 0, &ext2_mount, NULL, NULL
};

// static struct inode_operations ext2_i_op = {
//     &ext2_lookup, &ext2_open, NULL
// };

static struct inode_operations ext2_dir_i_op = {
    &ext2_lookup, &ext2_open, NULL
};

// static struct file_operations ext2_f_op = {
//     NULL, &ext2_read, &ext2_write
// };

// static struct file_operations ext2_dir_f_op = {
//     NULL, NULL, NULL
// };

/* Allocate space to read blocks (eventually should probably not use malloc and instead another mechanism) */
static void *ext2_allocate_blocks(struct super_block *sb, blkcnt_t num_blocks) {
    void * ret = malloc(num_blocks * sb->block_size);
    assert(ret);
    return ret;
}

/* Reads blocks at a given offset into the buffer */
static ssize_t ext2_read_blocks(struct super_block *sb, void *buffer, uint32_t block_offset, blkcnt_t num_blocks) {
    spin_lock(&sb->super_block_lock);

    sb->dev_file->position = sb->block_size * block_offset;
    debug_log("Ext2 dev file position: [ %#.16lX ]\n", sb->dev_file->position);
    ssize_t ret = fs_read(sb->dev_file, buffer, sb->block_size * num_blocks);

    spin_unlock(&sb->super_block_lock);
    return ret < 0 ? ret : ret / sb->block_size;
}

/* May need more complicated actions later */
static void ext2_free_blocks(void *buffer) {
    free(buffer);
}

struct tnode *ext2_lookup(struct inode *inode, const char *name) {
    assert(inode->flags & FS_DIR);
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

struct file *ext2_open(struct inode *inode, int *error) {
    (void) inode;

    *error = -EINVAL;
    return NULL;
}

ssize_t ext2_read(struct file *file, void *buffer, size_t len) {
    (void) file;
    (void) buffer;
    (void) len;

    return -EINVAL;
}

ssize_t ext2_write(struct file *file, const void *buffer, size_t len) {
    (void) file;
    (void) buffer;
    (void) len;

    return -EINVAL;
}

struct tnode *ext2_mount(struct file_system *current_fs, char *device_path) {
    int error = 0;
    struct file *dev_file = fs_open(device_path, &error);
    if (dev_file == NULL) {
        return NULL;
    }

    struct tnode *t_root = calloc(1, sizeof(struct tnode));
    struct inode *root = calloc(1, sizeof(struct inode));
    struct super_block *super_block = calloc(1, sizeof(struct super_block));

    super_block->device = 0;
    super_block->op = NULL;
    super_block->root = t_root;
    init_spinlock(&super_block->super_block_lock);
    super_block->block_size = EXT2_SUPER_BLOCK_SIZE; // Set this as defulat for first read
    super_block->dev_file = dev_file;

    struct ext2_raw_super_block *raw_super_block = ext2_allocate_blocks(super_block, 1);
    if (ext2_read_blocks(super_block, raw_super_block, EXT2_SUPER_BLOCK_OFFSET / EXT2_SUPER_BLOCK_SIZE, 1) != 1) {
        debug_log("Ext2 Read Error: [ Super Block ]\n");
        ext2_free_blocks(raw_super_block);
        return NULL;
    }

    super_block->block_size = 1024 << raw_super_block->shifted_blck_size;

    /* Other sizes are not supported */
    assert(super_block->block_size == 1024);

    struct raw_block_group_descriptor *raw_block_group_descriptor_table = ext2_allocate_blocks(super_block, 1);
    if (ext2_read_blocks(super_block, raw_block_group_descriptor_table, 2, 1) != 1) {
        debug_log("Ext2 Read Error: [ Block Group Descriptor Table ]\n");
        ext2_free_blocks(raw_super_block);
        ext2_free_blocks(raw_block_group_descriptor_table);
        return NULL;
    }

    debug_log("Inode Table Block Address: [ %u ]\n", raw_block_group_descriptor_table[2].inode_table_block_address);
    debug_log("Block Size: [ %lu ]\n", super_block->block_size);
    debug_log("LBA Address: [ %#.16lX ]\n", raw_block_group_descriptor_table[2].inode_table_block_address * super_block->block_size / 512);

    struct raw_inode *raw_inode_table = ext2_allocate_blocks(super_block, 1);
    if (ext2_read_blocks(super_block, raw_inode_table, raw_block_group_descriptor_table[2].inode_table_block_address, 1) != 1) {
        debug_log("Ext2 Read Error: [ Inode Table ]\n");
        ext2_free_blocks(raw_super_block);
        ext2_free_blocks(raw_block_group_descriptor_table);
        ext2_free_blocks(raw_inode_table);
        return NULL;
    }

    debug_log("Super Block First Non-Reserved Inode Location: [ %u ]\n", raw_super_block->first_non_reserved_inode);
    debug_log("Ext2 Inode Mode: [ %u ]\n", raw_inode_table[0].mode);
    debug_log("Ext2 Inode Location: [ %u ]\n", raw_inode_table[0].block[0]);

    ext2_free_blocks(raw_inode_table);
    ext2_free_blocks(raw_block_group_descriptor_table);
    ext2_free_blocks(raw_super_block);

    assert(strlen(device_path) != 0);

    t_root->inode = root;

    root->device = 0;
    root->flags = FS_DIR;
    root->i_op = &ext2_dir_i_op;
    root->index = fs_get_next_inode_id();
    init_spinlock(&root->lock);
    root->mode = S_IFDIR | 0777;
    root->mounts = NULL;
    root->private_data = NULL;
    root->size = 0;
    root->super_block = super_block;
    root->tnode_list = NULL;

    current_fs->super_block = super_block;

    return t_root;
}

void init_ext2() {
    load_fs(&fs);
}