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
    debug_log("Device path: [ %s ]\n", device_path);

    int error = 0;
    struct file *file = fs_open(device_path, &error);
    if (file == NULL) {
        return NULL;
    }

    /* Set to read starting from byte 1024 */
    file->position = EXT2_SUPER_BLOCK_OFFSET;

    struct ext2_raw_super_block *raw_super_block = malloc(EXT2_SUPER_BLOCK_SIZE);
    if (!fs_read(file, raw_super_block, EXT2_SUPER_BLOCK_SIZE)) {
        debug_log("Read Error\n");
    }

    debug_log("Num Inodes: [ %u ]\n", raw_super_block->num_inodes);
    debug_log("Num blocks: [ %u ]\n", raw_super_block->num_blocks);
    debug_log("Num reserved: [ %u ]\n", raw_super_block->num_reserved_blocks);
    debug_log("Num unallocated blocks: [ %u ]\n", raw_super_block->num_unallocated_blocks);
    debug_log("Num unallocated inodes: [ %u ]\n", raw_super_block->num_unallocated_inodes);
    debug_log("Block size: [ %u ]\n", 1024 << raw_super_block->shifted_blck_size);
    debug_log("Fragment size: [ %u ]\n", 1024 << raw_super_block->shifted_fragment_size);
    debug_log("Num blocks in group: [ %u ]\n", raw_super_block->num_blocks_in_block_group);
    debug_log("Num fragments in group: [ %u ]\n", raw_super_block->num_fragments_in_block_group);
    debug_log("Num inodes in group: [ %u ]\n", raw_super_block->num_inodes_in_block_group);
    debug_log("Ext2 signature: [ %#.4X ]\n", raw_super_block->ext2_sig);
    debug_log("Major version: [ %u ]\n", raw_super_block->version_major);
    debug_log("Inode size: [ %u ]\n", raw_super_block->inode_size);
    debug_log("Path of last mount: [ %s ]\n", raw_super_block->path_of_last_mount);
    
    assert(1024 << raw_super_block->shifted_blck_size == 1024);

    free(raw_super_block);

    // file->position = 1024 * 2;
    // uint32_t *raw_block_group_descriptor = malloc(1024);
    // if (!fs_read(file, raw_block_group_descriptor, 1024)) {
    //     debug_log("Read Error\n");
    // }

    // debug_log("Block Address: [ %u ]\n", *raw_block_group_descriptor);

    // free(raw_block_group_descriptor);
 
    struct tnode *t_root = calloc(1, sizeof(struct tnode));
    struct inode *root = calloc(1, sizeof(struct inode));
    struct super_block *super_block = calloc(1, sizeof(struct super_block));

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

    super_block->device = 0;
    super_block->op = NULL;
    super_block->root = t_root;

    current_fs->super_block = super_block;

    return t_root;
}

void init_ext2() {
    load_fs(&fs);
}