#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/param.h>
#include <errno.h>
#include <dirent.h>
#include <stdbool.h>

#include <kernel/fs/dev.h>
#include <kernel/fs/file.h>
#include <kernel/fs/inode.h>
#include <kernel/fs/inode_store.h>
#include <kernel/fs/ext2.h>
#include <kernel/fs/file_system.h>
#include <kernel/fs/vfs.h>
#include <kernel/fs/super_block.h>
#include <kernel/hal/output.h>
#include <kernel/util/spinlock.h>

static struct file_system fs = {
    "ext2", 0, &ext2_mount, NULL, NULL
};

static struct inode_operations ext2_i_op = {
    &ext2_lookup, &ext2_open, &ext2_stat
};

static struct inode_operations ext2_dir_i_op = {
    &ext2_lookup, &ext2_open, &ext2_stat
};

static struct file_operations ext2_f_op = {
    NULL, &ext2_read, &ext2_write
};

static struct file_operations ext2_dir_f_op = {
    NULL, NULL, NULL
};

static int inode_table_hash(void *index, int num_buckets) {
    return *((size_t*) index) % num_buckets;
}

static int inode_table_equals(void *i1, void *i2) {
    return *((size_t*) i1) == *((size_t*) i2);
}

static void *inode_table_key(void *inode_table) {
    return &((struct ext2_inode_table*) inode_table)->index;
}

/* Allocate space to read blocks (eventually should probably not use malloc and instead another mechanism) */
static void *ext2_allocate_blocks(struct super_block *sb, blkcnt_t num_blocks) {
    void * ret = malloc(num_blocks * sb->block_size);
    assert(ret);
    return ret;
}

/* Reads blocks at a given offset into the buffer */
static ssize_t ext2_read_blocks(struct super_block *sb, void *buffer, uint32_t block_offset, blkcnt_t num_blocks) {
    spin_lock(&sb->super_block_lock);

    for (blkcnt_t i = 0; i < num_blocks; i++) {
        sb->dev_file->position = sb->block_size * (block_offset + i);
        if (fs_read(sb->dev_file, (void*) (((uintptr_t) buffer) + sb->block_size * i), sb->block_size) != sb->block_size) {
            spin_unlock(&sb->super_block_lock);
            return -EIO;
        }
    }

    spin_unlock(&sb->super_block_lock);
    return num_blocks;
}

/* May need more complicated actions later */
static void ext2_free_blocks(void *buffer) {
    free(buffer);
}

/* Gets the block group a given inode is in */
static size_t ext2_get_block_group_from_inode(struct super_block *sb, uint32_t inode_id) {
    struct ext2_sb_data *data = sb->private_data;
    return (inode_id - 1) / data->sb->num_inodes_in_block_group;
}

/* Gets the relative index of an inode within a block group inode table */
static size_t ext2_get_inode_table_index(struct super_block *sb, uint32_t inode_id) {
    struct ext2_sb_data *data = sb->private_data;
    return (inode_id - 1) % data->sb->num_inodes_in_block_group;
}

/* Gets inode table for a given block group index */
static struct raw_inode *ext2_get_inode_table(struct super_block *sb, size_t index) {
    struct ext2_sb_data *data = sb->private_data;
    assert(index < data->num_block_groups);

    struct ext2_inode_table *group = hash_get(data->inode_table_map, &index);
    if (!group) {
        struct raw_block_group_descriptor block_group = data->blk_desc_table[index]; 
        ssize_t num_blocks = data->sb->num_inodes_in_block_group * sizeof(struct raw_inode) / sb->block_size;
        struct raw_inode *inode_table_start = ext2_allocate_blocks(sb, num_blocks);
        if (ext2_read_blocks(sb, inode_table_start, block_group.inode_table_block_address, num_blocks) != num_blocks) {
            debug_log("Ext2 Read Inode Table Failed: [ %lu ]\n", index);
            return NULL;
        }

        group = malloc(sizeof(struct ext2_inode_table));
        group->index = index;
        group->inode_table_start = inode_table_start;
        hash_put(data->inode_table_map, group);
    }

    return group->inode_table_start;
}

/* Gets the raw inode from index */
struct raw_inode *ext2_get_raw_inode(struct super_block *sb, uint32_t index) {
    struct raw_inode *inode_table = ext2_get_inode_table(sb, ext2_get_block_group_from_inode(sb, index));
    assert(inode_table);

    return inode_table + ext2_get_inode_table_index(sb, index);
}

static void ext2_update_inode(struct inode *inode, bool update_tnodes);

/* Reads dirent entries of an inode */
static void ext2_update_tnode_list(struct inode *inode) {
    debug_log("Updating Tnode List: [ %llu ]\n", inode->index);

    if (!inode->private_data) {
        ext2_update_inode(inode, false);
    }

    struct raw_inode *raw_inode = inode->private_data;
    assert(raw_inode);

    struct raw_dirent *raw_dirent_table = ext2_allocate_blocks(inode->super_block, 1);
    if (!ext2_read_blocks(inode->super_block, raw_dirent_table, raw_inode->block[0], 1)) {
        debug_log("Ext2 Read Error: [ %llu ]\n", inode->index);
        return;
    }

    spin_lock(&inode->lock);

    size_t block_no = 0;
    struct raw_dirent *dirent = raw_dirent_table;
    for (;;) {
        if (dirent->type == EXT2_DIRENT_TYPE_UNKNOWN || strcmp(dirent->name, "kernel.map") == 0) {
            break;
        }

        struct tnode *tnode = malloc(sizeof(struct tnode));
        struct inode *inode_to_add = calloc(1, sizeof(struct inode));

        tnode->name = calloc(dirent->name_length + 1, sizeof(char));
        memcpy(tnode->name, dirent->name, dirent->name_length);
        tnode->inode = inode_to_add;
        inode->tnode_list = add_tnode(inode->tnode_list, tnode);

        inode_to_add->device = inode->device;
        inode_to_add->parent = inode == inode->super_block->root->inode ? inode->super_block->root : find_tnode_inode(inode->parent->inode->tnode_list, inode);
        assert(inode_to_add->parent->inode == inode);
        inode_to_add->index = dirent->ino;
        inode_to_add->i_op = &ext2_i_op;
        inode_to_add->super_block = inode->super_block;
        inode_to_add->flags = dirent->type == EXT2_DIRENT_TYPE_DIRECTORY ? FS_DIR : FS_FILE;
        init_spinlock(&inode_to_add->lock);

        dirent = EXT2_NEXT_DIRENT(dirent);
        free(NULL);
        if ((uintptr_t) dirent >= ((uintptr_t) raw_dirent_table) + inode->super_block->block_size) {
            ext2_free_blocks(raw_dirent_table);
            block_no++;

            /* Can't read the indirect blocks */
            if (block_no >= 12) {
                break;
            }

            raw_dirent_table = ext2_allocate_blocks(inode->super_block, 1);
            if (ext2_read_blocks(inode->super_block, raw_dirent_table, raw_inode->block[block_no], 1) != 1) {
                ext2_free_blocks(raw_dirent_table);
                break;
            }

            dirent = raw_dirent_table;
        }
    }

    spin_unlock(&inode->lock);

    ext2_free_blocks(raw_dirent_table);
}

/* Reads raw inode info into memory and updates inode */
static void ext2_update_inode(struct inode *inode, bool update_tnodes) {
    assert(inode->private_data == NULL);

    struct raw_inode *raw_inode = ext2_get_raw_inode(inode->super_block, inode->index);
    assert(raw_inode);
    inode->private_data = raw_inode;

    if (update_tnodes && inode->flags & FS_DIR && inode->tnode_list == NULL) {
        ext2_update_tnode_list(inode);
    }

    inode->mode = raw_inode->mode;
    inode->size = raw_inode->size;
}

struct tnode *ext2_lookup(struct inode *inode, const char *name) {
    assert(inode->flags & FS_DIR);

    if (inode->tnode_list == NULL) {
        ext2_update_tnode_list(inode);
    }

    if (name == NULL) {
        return NULL;
    }

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
    if (!inode->private_data) {
        ext2_update_inode(inode, true);
    }

    struct raw_inode *raw_inode = inode->private_data;

    struct file *file = calloc(1, sizeof(struct file));
    file->device = inode->device;
    file->f_op = inode->flags & FS_DIR ? &ext2_dir_f_op : &ext2_f_op;
    file->flags = inode->flags;
    file->inode_idenifier = inode->index;
    file->length = raw_inode->size;
    file->position = 0;
    file->start = 0;

    *error = 0;
    return file;
}

/* Should provide some sort of mechanism for caching these blocks */
ssize_t ext2_read(struct file *file, void *buffer, size_t len) {
    assert(file->flags & FS_FILE);
    assert(len > 1);

    struct inode *inode = fs_inode_get(file->device, file->inode_idenifier);
    assert(inode);
    assert(inode->private_data);

    size_t max_can_read = inode->size - file->position + 1;
    len = MIN(len, max_can_read) - 1;

    size_t file_block_no = file->position / inode->super_block->block_size;
    size_t file_block_no_end = (file->position + len + inode->super_block->block_size - 1) / inode->super_block->block_size;
    
    debug_log("Reading Inode: [ %llu, %lu, %lu, %lu ]\n", inode->index, len, file_block_no, file_block_no_end);

    while (file_block_no < file_block_no_end) {
        void *block = ext2_allocate_blocks(inode->super_block, 1);

        /* Not supporting the indirect blocks yet */
        if (file_block_no >= 12) {
            ext2_free_blocks(block);
            return -EINVAL;
        }

        ssize_t ret = ext2_read_blocks(inode->super_block, block, ((struct raw_inode*) inode->private_data)->block[file_block_no], 1);
        if (ret != 1) {
            return ret;
        }

        size_t buffer_offset = file->position % inode->super_block->block_size;
        size_t to_read = MIN(inode->super_block->block_size - buffer_offset, len);

        memcpy(buffer, (void*) (((uintptr_t) block) + buffer_offset), to_read);
        file->position += buffer_offset;
        len -= buffer_offset;

        ext2_free_blocks(block);
        file_block_no++;
        buffer = (void*) (((uintptr_t) buffer) + inode->super_block->block_size);
    }

    ((char*) buffer)[len] = '\0';
    return len + 1;
}

ssize_t ext2_write(struct file *file, const void *buffer, size_t len) {
    (void) file;
    (void) buffer;
    (void) len;

    return -EINVAL;
}

int ext2_stat(struct inode *inode, struct stat *stat_struct) {
    if (!inode->private_data) {
        ext2_update_inode(inode, false);
    }

    stat_struct->st_size = inode->size;
    stat_struct->st_blocks = (inode->size / inode->super_block->block_size - 1) + 1;
    stat_struct->st_blksize = inode->super_block->block_size;
    stat_struct->st_ino = inode->index;
    stat_struct->st_dev = inode->device;
    stat_struct->st_mode = inode->mode;
    stat_struct->st_rdev = 0;

    return 0;
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
    struct ext2_sb_data *data = calloc(1, sizeof(struct ext2_sb_data));

    super_block->device = dev_get_device_number(dev_file);
    super_block->op = NULL;
    super_block->root = t_root;
    init_spinlock(&super_block->super_block_lock);
    super_block->block_size = EXT2_SUPER_BLOCK_SIZE; // Set this as defulat for first read
    super_block->dev_file = dev_file;
    super_block->private_data = data;
    data->inode_table_map = hash_create_hash_map(inode_table_hash, inode_table_equals, inode_table_key);

    struct ext2_raw_super_block *raw_super_block = ext2_allocate_blocks(super_block, 1);
    if (ext2_read_blocks(super_block, raw_super_block, EXT2_SUPER_BLOCK_OFFSET / EXT2_SUPER_BLOCK_SIZE, 1) != 1) {
        debug_log("Ext2 Read Error: [ Super Block ]\n");
        ext2_free_blocks(raw_super_block);
        return NULL;
    }

    debug_log("Ext2 Num Inodes in Block Group: [ %u ]\n", raw_super_block->num_inodes_in_block_group);
    debug_log("Ext2 Num Blocks: [ %#.8X ]\n", raw_super_block->num_blocks);
    debug_log("Ext2 Num Inodes: [ %#.8X ]\n", raw_super_block->num_inodes);

    data->sb = raw_super_block;
    data->num_block_groups = (raw_super_block->num_blocks + raw_super_block->num_blocks_in_block_group - 1) / raw_super_block->num_blocks_in_block_group;
    super_block->block_size = 1024 << raw_super_block->shifted_blck_size;

    /* Other sizes are not supported */
    assert(super_block->block_size == 1024);

    blkcnt_t num_blocks = (data->num_block_groups * sizeof(struct raw_block_group_descriptor) + super_block->block_size - 1) / super_block->block_size;
    struct raw_block_group_descriptor *raw_block_group_descriptor_table = ext2_allocate_blocks(super_block, num_blocks);
    if (ext2_read_blocks(super_block, raw_block_group_descriptor_table, 2, num_blocks) != num_blocks) {
        debug_log("Ext2 Read Error: [ Block Group Descriptor Table ]\n");
        ext2_free_blocks(raw_super_block);
        ext2_free_blocks(raw_block_group_descriptor_table);
        return NULL;
    }

    data->blk_desc_table = raw_block_group_descriptor_table;
    assert(strlen(device_path) != 0);

    t_root->inode = root;

    root->device = super_block->device;
    root->flags = FS_DIR;
    root->i_op = &ext2_dir_i_op;
    root->index = EXT2_ROOT_INODE;
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