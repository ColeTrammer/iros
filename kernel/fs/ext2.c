#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/param.h>
#include <errno.h>
#include <dirent.h>
#include <stdbool.h>
#include <fcntl.h>

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
    NULL, &ext2_lookup, &ext2_open, &ext2_stat, NULL, NULL, &ext2_unlink, NULL, &ext2_chmod
};

static struct inode_operations ext2_dir_i_op = {
    &ext2_create, &ext2_lookup, &ext2_open, &ext2_stat, NULL, &ext2_mkdir, NULL, &ext2_rmdir, &ext2_chmod
};

static struct file_operations ext2_f_op = {
    NULL, &ext2_read, &ext2_write, NULL
};

static struct file_operations ext2_dir_f_op = {
    NULL, NULL, NULL, NULL
};

static int block_group_hash(void *index, int num_buckets) {
    return *((size_t*) index) % num_buckets;
}

static int block_group_equals(void *i1, void *i2) {
    return *((size_t*) i1) == *((size_t*) i2);
}

static void *block_group_key(void *block_group) {
    return &((struct ext2_block_group*) block_group)->index;
}

static int block_hash(void *index, int num_buckets) {
    return *((uint32_t*) index) % num_buckets;
}

static int block_equals(void *i1, void *i2) {
    return *((uint32_t*) i1) == *((uint32_t*) i2);
}

static void *block_key(void *block_obj) {
    return &((struct ext2_block*) block_obj)->index;
}

/* Allocate space to read blocks (eventually should probably not use malloc and instead another mechanism) */
static void *ext2_allocate_blocks(struct super_block *sb, blkcnt_t num_blocks) {
    void * ret = malloc(num_blocks * sb->block_size);
    assert(ret);
    return ret;
}

#define MAX_BLOCKS_TO_CACHE 1000

/* Reads blocks at a given offset into the buffer */
static ssize_t ext2_read_blocks(struct super_block *sb, void *buffer, uint32_t block_offset, blkcnt_t num_blocks) {
    struct ext2_sb_data *data = sb->private_data;
    spin_lock(&sb->super_block_lock);

    struct ext2_block *block = NULL;
    if (num_blocks == 1 && (block = hash_get(data->block_map, &block_offset))) {
        memcpy(buffer, block->block, sb->block_size);
        spin_unlock(&sb->super_block_lock);
        return num_blocks;
    }

    sb->dev_file->position = sb->block_size * block_offset;
    if (fs_read(sb->dev_file, (void*) buffer, num_blocks * sb->block_size) != num_blocks * sb->block_size) {
        spin_unlock(&sb->super_block_lock);
        return -EIO;
    }

    if (num_blocks == 1 && data->num_blocks_cached < MAX_BLOCKS_TO_CACHE) {
        block = malloc(sizeof(struct ext2_block));
        block->index = block_offset;
        block->block = ext2_allocate_blocks(sb, 1);
        memcpy(block->block, buffer, sb->block_size);
        hash_put(data->block_map, block);
    }

    spin_unlock(&sb->super_block_lock);
    return num_blocks;
}

/* Writes to blocks at a given offset from buffer */
static ssize_t ext2_write_blocks(struct super_block *sb, const void *buffer, uint32_t block_offset, blkcnt_t num_blocks) {
    struct ext2_sb_data *data = sb->private_data;
    spin_lock(&sb->super_block_lock);

    sb->dev_file->position = sb->block_size * block_offset;
    if (fs_write(sb->dev_file, buffer, num_blocks * sb->block_size) != num_blocks * sb->block_size) {
        spin_unlock(&sb->super_block_lock);
        return -EIO;
    }

    if (num_blocks == 1) {
        struct ext2_block *block = hash_get(data->block_map, &block_offset);
        if (block != NULL) {
            memcpy(block->block, buffer, sb->block_size);
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

/* Gets block group object for a given block group index */
static struct ext2_block_group *ext2_get_block_group(struct super_block *super_block, size_t index) {
    struct ext2_sb_data *data = super_block->private_data;
    struct ext2_block_group *group = hash_get(data->block_group_map, &index);
    if (!group) {
        group = calloc(1, sizeof(struct ext2_block_group));
        group->index = index;
        group->blk_desc = data->blk_desc_table + index;
        hash_put(data->block_group_map, group);
    }

    return group;
}

/* Gets block usage bitmap for a given block group index */
static struct ext2_block_bitmap *ext2_get_block_usage_bitmap(struct super_block *sb, size_t index) {
    struct ext2_sb_data *data = sb->private_data;
    assert(index < data->num_block_groups);

    struct ext2_block_group *group = ext2_get_block_group(sb, index);

    if (!group->block_bitmap.bitmap) {
        ssize_t num_blocks = (data->sb->num_blocks_in_block_group + sb->block_size - 1) / sb->block_size;
        group->block_bitmap.num_bits = data->sb->num_blocks_in_block_group;
        group->block_bitmap.bitmap = ext2_allocate_blocks(sb, num_blocks);
        if (ext2_read_blocks(sb, group->block_bitmap.bitmap, group->blk_desc->block_usage_bitmap_block_address, num_blocks) != num_blocks) {
            debug_log("Ext2 Read block usage bitmap failed: [ %lu ]\n", index);
            ext2_free_blocks(group->block_bitmap.bitmap);
            return NULL;
        }
    }

    return &group->block_bitmap;
}

/* Finds the next open block in a particular block group bitmap */
static int64_t ext2_find_open_block_in_bitmap(struct ext2_block_bitmap *bitmap) {
    for (size_t i = 0; i < bitmap->num_bits / sizeof(uint64_t); i++) {
        /* Means at least one bit is unset */
        if (~bitmap->bitmap[i]) {
            for (uint64_t test = 0; test < 64; test++) {
                if (!(bitmap->bitmap[i] & (1UL << test))) {
                    return i * sizeof(uint64_t) * 8 + test;
                }
            }
        }
    }

    return -1;
}

/* Syncs block group specified by blk_grp_index */
static int ext2_sync_block_group(struct super_block *sb, uint32_t blk_grp_index) {
    struct ext2_sb_data *data = sb->private_data;
    size_t block_off =  blk_grp_index * sizeof(struct raw_block_group_descriptor) / sb->block_size;
    size_t raw_off = block_off * sb->block_size;

    ssize_t ret = ext2_write_blocks(sb,
                                    (void*) (((uintptr_t) (data->blk_desc_table)) + raw_off),
                                    2 + block_off,
                                    1

    );

    if (ret != 1) {
        return (int) ret;
    }

    return 0;
}

/* Syncs super_block to disk */
static int ext2_sync_super_block(struct super_block *sb) {
    struct ext2_sb_data *data = sb->private_data;

    ssize_t ret = ext2_write_blocks(sb, data->sb, 1, 1);

    if (ret < 1) {
        return (int) ret;
    }

    return 0;
}

/*
    Note: block indexes are very confusing... It seems like the first bit in the 
    block bitmap corresponds to block 1, not block 0 (this is plausible since
    block 0 is completely unused in the ext2 fs). However, reading and writing
    blocks must take block 0 into account. Therefore, block indexes are adjusted
    in the functions that deal with block allocations, but this might be incorrect
    behavior.
*/

/* Sets a block index as allocated */
static int ext2_set_block_allocated(struct super_block *super_block, uint32_t index) {
    index--;
    struct ext2_sb_data *data = super_block->private_data;
    struct ext2_block_bitmap *bitmap = ext2_get_block_usage_bitmap(super_block, index / data->sb->num_blocks_in_block_group);
    size_t rel_offset = index % data->sb->num_blocks_in_block_group;
    size_t off_64 = rel_offset / sizeof(uint64_t) / 8;
    size_t off_bit = rel_offset % (sizeof(uint64_t) * 8);
    bitmap->bitmap[off_64] |= 1UL << off_bit;

    size_t block_off = off_64 * sizeof(uint64_t) / super_block->block_size;
    ssize_t ret = ext2_write_blocks(super_block, 
                                    bitmap->bitmap + (block_off * super_block->block_size / sizeof(uint64_t)),
                                    ((struct ext2_sb_data*) super_block->private_data)->blk_desc_table[index / data->sb->num_blocks_in_block_group].block_usage_bitmap_block_address + block_off,
                                    1);
    if (ret != 1) {
        return (int) ret;
    }

    struct ext2_block_group *block_group = ext2_get_block_group(super_block, index / data->sb->num_blocks_in_block_group);
    block_group->blk_desc->num_unallocated_blocks--;
    ret = ext2_sync_block_group(super_block, index / data->sb->num_blocks_in_block_group);

    if (ret != 0) {
        return (int) ret;
    }

    data->sb->num_unallocated_blocks--;
    return ext2_sync_super_block(super_block);
}

/* Frees a block from block bitmap */
static int ext2_free_block(struct super_block *super_block, uint32_t index, bool sync_bookkeeping_fields) {
    index--;
    struct ext2_sb_data *data = super_block->private_data;
    struct ext2_block_bitmap *bitmap = ext2_get_block_usage_bitmap(super_block, index / data->sb->num_blocks_in_block_group);
    size_t rel_offset = index % data->sb->num_blocks_in_block_group;
    size_t off_64 = rel_offset / sizeof(uint64_t) / 8;
    size_t off_bit = rel_offset % (sizeof(uint64_t) * 8);
    bitmap->bitmap[off_64] &= ~(1UL << off_bit);

    size_t block_off = off_64 * sizeof(uint64_t) / super_block->block_size;
    ssize_t ret = ext2_write_blocks(super_block, 
                                    bitmap->bitmap + (block_off * super_block->block_size / sizeof(uint64_t)),
                                    ((struct ext2_sb_data*) super_block->private_data)->blk_desc_table[index / data->sb->num_blocks_in_block_group].block_usage_bitmap_block_address + block_off,
                                    1);
    if (ret != 1) {
        return (int) ret;
    }

    if (sync_bookkeeping_fields) {
        struct ext2_block_group *block_group = ext2_get_block_group(super_block, index / data->sb->num_blocks_in_block_group);
        block_group->blk_desc->num_unallocated_blocks--;
        ret = ext2_sync_block_group(super_block, index / data->sb->num_blocks_in_block_group);

        if (ret != 0) {
            return (int) ret;
        }

        data->sb->num_unallocated_blocks--;
        return ext2_sync_super_block(super_block);
    }

    return 0;
}

/* Find an open block (using usage bitmap), starting with the one specified by blk_grp_index */
static uint32_t ext2_find_open_block(struct super_block *sb, size_t blk_grp_index) {
    struct ext2_sb_data *data = sb->private_data;
    int64_t ret = ext2_find_open_block_in_bitmap(ext2_get_block_usage_bitmap(sb, blk_grp_index));
    if (ret < 0) {
        size_t save_blk_grp_index = blk_grp_index;
        for (blk_grp_index = 0; blk_grp_index < data->num_block_groups; blk_grp_index++) {
            if (blk_grp_index == save_blk_grp_index) { continue; }

            ret = ext2_find_open_block_in_bitmap(ext2_get_block_usage_bitmap(sb, blk_grp_index));
            
            if (ret >= 0) { 
                break; 
            }
        }
    }

    if (ret < 0) {
        return 0;
    }

    ret += data->sb->num_inodes_in_block_group * blk_grp_index + 1;
    debug_log("Allocated block index: [ %ld ]\n", ret);
    return (uint32_t) ret;
}

/* Gets inode usage bitmap for a given block group index */
static struct ext2_inode_bitmap *ext2_get_inode_usage_bitmap(struct super_block *sb, size_t index) {
    struct ext2_sb_data *data = sb->private_data;
    assert(index < data->num_block_groups);

    struct ext2_block_group *group = ext2_get_block_group(sb, index);

    if (!group->inode_bitmap.bitmap) {
        ssize_t num_blocks = (data->sb->num_inodes_in_block_group + sb->block_size - 1) / sb->block_size;
        group->inode_bitmap.num_bits = data->sb->num_inodes_in_block_group;
        group->inode_bitmap.bitmap = ext2_allocate_blocks(sb, num_blocks);
        if (ext2_read_blocks(sb, group->inode_bitmap.bitmap, group->blk_desc->inode_usage_bitmap_block_address, num_blocks) != num_blocks) {
            debug_log("Ext2 Read block usage bitmap failed: [ %lu ]\n", index);
            ext2_free_blocks(group->inode_bitmap.bitmap);
            return NULL;
        }
    }
    return &group->inode_bitmap;
}

/* Finds the next open inode in a particular block group bitmap */
static int64_t ext2_find_open_inode_in_bitmap(struct ext2_inode_bitmap *bitmap) {
    for (size_t i = 0; i < bitmap->num_bits / sizeof(uint64_t); i++) {
        /* Means at least one bit is unset */
        if (~bitmap->bitmap[i]) {
            for (uint64_t test = 0; test < 64; test++) {
                if (!(bitmap->bitmap[i] & (1UL << test))) {
                    return i * sizeof(uint64_t) * 8 + test;
                }
            }
        }
    }

    return -1;
}

/* Sets a inode index as allocated */
static int ext2_set_inode_allocated(struct super_block *super_block, uint32_t index) {
    struct ext2_inode_bitmap *bitmap = ext2_get_inode_usage_bitmap(super_block, ext2_get_block_group_from_inode(super_block, index));
    size_t rel_offset = ext2_get_inode_table_index(super_block, index);
    size_t off_64 = rel_offset / sizeof(uint64_t) / 8;
    size_t off_bit = rel_offset % (sizeof(uint64_t) * 8);
    bitmap->bitmap[off_64] |= 1UL << off_bit;

    size_t block_off = off_64 * sizeof(uint64_t) / super_block->block_size;
    ssize_t ret = ext2_write_blocks(super_block, 
                                    bitmap->bitmap + (block_off * super_block->block_size / sizeof(uint64_t)),
                                    ((struct ext2_sb_data*) super_block->private_data)->blk_desc_table[ext2_get_block_group_from_inode(super_block, index)].inode_usage_bitmap_block_address + block_off,
                                    1);
    if (ret != 1) {
        return (int) ret;
    }

    struct ext2_block_group *block_group = ext2_get_block_group(super_block, ext2_get_block_group_from_inode(super_block, index));
    block_group->blk_desc->num_unallocated_inodes--;
    ret = ext2_sync_block_group(super_block, ext2_get_block_group_from_inode(super_block, index));

    if (ret != 0) {
        return (int) ret;
    }

    struct ext2_sb_data *sb_data = super_block->private_data;
    sb_data->sb->num_unallocated_inodes--;
    return ext2_sync_super_block(super_block);
}

/* Frees an inode index in the inode bitmap */
static int ext2_free_inode(struct super_block *super_block, uint32_t index, bool sync_bookkeeping_fields) {
    struct ext2_inode_bitmap *bitmap = ext2_get_inode_usage_bitmap(super_block, ext2_get_block_group_from_inode(super_block, index));
    size_t rel_offset = ext2_get_inode_table_index(super_block, index);
    size_t off_64 = rel_offset / sizeof(uint64_t) / 8;
    size_t off_bit = rel_offset % (sizeof(uint64_t) * 8);
    bitmap->bitmap[off_64] &= ~(1UL << off_bit);

    size_t block_off = off_64 * sizeof(uint64_t) / super_block->block_size;
    ssize_t ret = ext2_write_blocks(super_block, 
                                    bitmap->bitmap + (block_off * super_block->block_size / sizeof(uint64_t)),
                                    ((struct ext2_sb_data*) super_block->private_data)->blk_desc_table[ext2_get_block_group_from_inode(super_block, index)].inode_usage_bitmap_block_address + block_off,
                                    1);
    if (ret != 1) {
        return (int) ret;
    }

    if (sync_bookkeeping_fields) {
        struct ext2_block_group *block_group = ext2_get_block_group(super_block, ext2_get_block_group_from_inode(super_block, index));
        block_group->blk_desc->num_unallocated_inodes--;
        ret = ext2_sync_block_group(super_block, ext2_get_block_group_from_inode(super_block, index));

        if (ret != 0) {
            return (int) ret;
        }

        struct ext2_sb_data *sb_data = super_block->private_data;
        sb_data->sb->num_unallocated_inodes--;
        return ext2_sync_super_block(super_block);
    }

    return 0;
}

/* Find an open inode (using usage bitmap), starting with the one specified by blk_grp_index */
static uint32_t ext2_find_open_inode(struct super_block *sb, size_t blk_grp_index) {
    struct ext2_sb_data *data = sb->private_data;
    int64_t ret = ext2_find_open_inode_in_bitmap(ext2_get_inode_usage_bitmap(sb, blk_grp_index));
    if (ret < 0) {
        size_t save_blk_grp_index = blk_grp_index;
        for (blk_grp_index = 0; blk_grp_index < data->num_block_groups; blk_grp_index++) {
            if (blk_grp_index == save_blk_grp_index) { continue; }

            ret = ext2_find_open_inode_in_bitmap(ext2_get_inode_usage_bitmap(sb, blk_grp_index));
            
            if (ret >= 0) { 
                break; 
            }
        }
    }

    if (ret < 0) {
        return 0;
    }

    ret += data->sb->num_inodes_in_block_group * blk_grp_index + 1;
    debug_log("Allocated inode index: [ %ld ]\n", ret);
    return (uint32_t) ret;
}

/* Gets inode table for a given block group index */
static struct raw_inode *ext2_get_inode_table(struct super_block *sb, size_t index) {
    struct ext2_sb_data *data = sb->private_data;
    assert(index < data->num_block_groups);

    struct ext2_block_group *group = ext2_get_block_group(sb, index);

    if (!group->inode_table_start) {
        ssize_t num_blocks = (data->sb->num_inodes_in_block_group * sizeof(struct raw_inode) + sb->block_size - 1) / sb->block_size;
        struct raw_inode *inode_table_start = ext2_allocate_blocks(sb, num_blocks);
        if (ext2_read_blocks(sb, inode_table_start, group->blk_desc->inode_table_block_address, num_blocks) != num_blocks) {
            debug_log("Ext2 Read Inode Table Failed: [ %lu ]\n", index);
            ext2_free_blocks(inode_table_start);
            return NULL;
        }

        group->inode_table_start = inode_table_start;
    }

    return group->inode_table_start;
}

/* Gets the raw inode from index */
static struct raw_inode *ext2_get_raw_inode(struct super_block *sb, uint32_t index) {
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

    for (; block_no * inode->super_block->block_size < inode->size ;) {
        if (dirent->ino == 0 || dirent->type == EXT2_DIRENT_TYPE_UNKNOWN) {
            goto get_next_dirent;
        }

        struct tnode *tnode = malloc(sizeof(struct tnode));
        struct inode *inode_to_add;
        if (strcmp(dirent->name, ".") == 0) {
            inode_to_add = inode;
        } else if (strcmp(dirent->name, "..") == 0) {
            inode_to_add = inode->parent->inode;
        } else {
            inode_to_add = calloc(1, sizeof(struct inode));
            inode_to_add->device = inode->device;
            inode_to_add->parent = inode == inode->super_block->root->inode ? inode->super_block->root : find_tnode_inode(inode->parent->inode->tnode_list, inode);
            assert(inode_to_add->parent->inode == inode);
            inode_to_add->index = dirent->ino;
            inode_to_add->i_op = dirent->type == EXT2_DIRENT_TYPE_REGULAR ?  &ext2_i_op : &ext2_dir_i_op;
            inode_to_add->super_block = inode->super_block;
            inode_to_add->flags = dirent->type == EXT2_DIRENT_TYPE_REGULAR ? FS_FILE : FS_DIR;
            inode_to_add->ref_count = 1;
            init_spinlock(&inode_to_add->lock);
        }

        tnode->name = calloc(dirent->name_length + 1, sizeof(char));
        memcpy(tnode->name, dirent->name, dirent->name_length);
        tnode->inode = inode_to_add;
        inode->tnode_list = add_tnode(inode->tnode_list, tnode);

    get_next_dirent:
        dirent = EXT2_NEXT_DIRENT(dirent);
        if ((uintptr_t) dirent >= ((uintptr_t) raw_dirent_table) + inode->super_block->block_size) {
            if ((uintptr_t) dirent - (uintptr_t) raw_dirent_table >= inode->size) {
                break;
            }

            ext2_free_blocks(raw_dirent_table);
            block_no++;

            /* Can't read the indirect blocks */
            if (block_no >= EXT2_SINGLY_INDIRECT_BLOCK_INDEX) {
                raw_dirent_table = ext2_allocate_blocks(inode->super_block, 1);
                break;
            }

            raw_dirent_table = ext2_allocate_blocks(inode->super_block, 1);
            if (ext2_read_blocks(inode->super_block, raw_dirent_table, raw_inode->block[block_no], 1) != 1) {
                break;
            }

            dirent = raw_dirent_table;
        }
    }

    spin_unlock(&inode->lock);

    debug_log("Finished updating tnode list: [ %llu ]\n", inode->index);

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

/* Syncs raw_inode to disk */
static int ext2_sync_inode(struct inode *inode) {
    size_t inode_table_index = ext2_get_inode_table_index(inode->super_block, inode->index);
    size_t block_off = ((inode_table_index) * sizeof(struct raw_inode)) / inode->super_block->block_size;
    size_t block_group = ext2_get_block_group_from_inode(inode->super_block, inode->index);
    size_t raw_offset = block_off * inode->super_block->block_size;
    struct raw_inode *raw_inode_table = ext2_get_inode_table(inode->super_block, block_group);

    assert(((struct ext2_sb_data*) inode->super_block->private_data)->sb->inode_size == sizeof(struct raw_inode));
    assert(inode->private_data == raw_inode_table + inode_table_index);
    raw_inode_table[inode_table_index].size = inode->size;
    raw_inode_table[inode_table_index].mode = inode->mode;
    /* Sector size should be retrieved from block device */
    raw_inode_table[inode_table_index].sectors = (inode->size + 511) / 512;

    ssize_t ret = ext2_write_blocks(inode->super_block,
                                    (void*) (((uintptr_t) raw_inode_table) + raw_offset),
                                    ((struct ext2_sb_data*) inode->super_block->private_data)->blk_desc_table[block_group].inode_table_block_address + block_off,
                                    1);

    if (ret != 1) {
        return (int) ret;
    }

    return 0;
}

/* Handles the allocation of a block for an inode */
static int ext2_allocate_block_for_inode(struct super_block *sb, struct inode *inode, size_t block_no) {
    struct raw_inode *raw_inode = inode->private_data;

    if (block_no < EXT2_SINGLY_INDIRECT_BLOCK_INDEX) {
        raw_inode->block[block_no] = ext2_find_open_block(sb, ext2_get_block_group_from_inode(sb, inode->index));
        return ext2_set_block_allocated(sb, raw_inode->block[block_no]);
    }

    /* Handle up to singly indirect case */
    size_t real_block_no = block_no - EXT2_SINGLY_INDIRECT_BLOCK_INDEX;
    assert(real_block_no * sizeof(uint32_t) < (size_t) sb->block_size);

    if (raw_inode->block[EXT2_SINGLY_INDIRECT_BLOCK_INDEX] == 0) {
        raw_inode->block[EXT2_SINGLY_INDIRECT_BLOCK_INDEX] = ext2_find_open_block(sb, ext2_get_block_group_from_inode(sb, inode->index));
        int ret = ext2_set_block_allocated(sb, raw_inode->block[EXT2_SINGLY_INDIRECT_BLOCK_INDEX]);
        if (ret != 0) {
            return ret;
        }
    }

    uint32_t *block = ext2_allocate_blocks(sb, 1);
    ssize_t ret = ext2_read_blocks(sb, block, raw_inode->block[EXT2_SINGLY_INDIRECT_BLOCK_INDEX], 1);
    if (ret < 0) {
        ext2_free_blocks(block);
        return (int) ret;
    }

    block[real_block_no] = ext2_find_open_block(sb, ext2_get_block_group_from_inode(sb, inode->index));
    ret = ext2_set_block_allocated(sb, block[real_block_no]);
    if (ret < 0) {
        ext2_free_blocks(block);
        return (int) ret;
    }

    ret = ext2_write_blocks(sb, block, raw_inode->block[EXT2_SINGLY_INDIRECT_BLOCK_INDEX], 1);

    ext2_free_blocks(block);
    return ret;
}

/* Creates a raw_inode and syncs to disk */
static int ext2_write_inode(struct inode *inode) {
    assert(inode->private_data == NULL);
    assert(inode->mode != 0);

    struct ext2_sb_data *data = inode->super_block->private_data;
    struct raw_inode *raw_inode = ext2_get_raw_inode(inode->super_block, inode->index);
    assert(raw_inode);
    inode->private_data = raw_inode;

    raw_inode->mode = inode->mode;
    raw_inode->uid = 0;
    raw_inode->size = 0;
    raw_inode->atime = 0;
    raw_inode->ctime = 0;
    raw_inode->mtime = 0;
    raw_inode->dtime = 0;
    raw_inode->gid = 0;
    raw_inode->link_count = S_ISDIR(inode->mode) ? 2 : 1;
    raw_inode->sectors = 0;
    raw_inode->flags = 0;
    raw_inode->os_specific_1 = 0;
    ssize_t to_preallocate = S_ISREG(inode->mode) ? data->sb->num_blocks_to_preallocate_for_files : data->sb->num_blocks_to_preallocate_for_directories;
    void *zeroes = ext2_allocate_blocks(inode->super_block, 1);
    memset(zeroes, 0, inode->super_block->block_size);
    for (ssize_t i = 0; i < to_preallocate; i++) {
        int ret = ext2_allocate_block_for_inode(inode->super_block, inode, i);
        if (ret != 0) {
            return ret;
        }
    }
    ext2_free_blocks(zeroes);
    memset(raw_inode->block + MIN(EXT2_SINGLY_INDIRECT_BLOCK_INDEX, to_preallocate), 0, (MAX(15 - to_preallocate, 2)) * sizeof(uint32_t));
    raw_inode->generation = 0;
    raw_inode->file_acl = 0;
    raw_inode->dir_acl = 0;
    raw_inode->faddr = 0;
    memset(raw_inode->os_specific_2, 0, 12 * sizeof(uint8_t));

    return ext2_sync_inode(inode);
}

struct inode *ext2_create(struct tnode *tparent, const char *name, mode_t mode, int *error) {
    struct inode *parent = tparent->inode;
    uint32_t index = ext2_find_open_inode(parent->super_block, ext2_get_block_group_from_inode(parent->super_block, parent->index));
    if (index == 0) {
        *error = -ENOSPC;
        return NULL;
    }

    *error = ext2_set_inode_allocated(parent->super_block, index);
    if (*error != 0) {
        return NULL;
    }

    struct inode *inode = calloc(1, sizeof(struct inode));
    inode->device = parent->device;
    inode->flags = fs_mode_to_flags(mode);
    inode->i_op = S_ISDIR(mode) ? &ext2_dir_i_op : &ext2_i_op;
    inode->index = index;
    init_spinlock(&inode->lock);
    inode->mode = mode;
    inode->mounts = NULL;
    inode->parent = tparent;
    inode->private_data = NULL;
    inode->ref_count = 1;
    inode->size = 0;
    inode->super_block = parent->super_block;
    inode->tnode_list = NULL;

    ext2_write_inode(inode);

    if (parent->private_data == NULL) {
        ext2_update_inode(parent, true);
    }

    struct raw_inode *parent_raw_inode = parent->private_data;
    struct raw_dirent *raw_dirent_table = ext2_allocate_blocks(inode->super_block, 1);
    ssize_t ret = ext2_read_blocks(inode->super_block, raw_dirent_table, parent_raw_inode->block[0], 1);
    if (ret != 1) {
        /* We're screwed at this point */
        debug_log("Ext2 read error (reading dirents): [ %llu ]\n", parent->index);
        *error = (int) ret;
        ext2_free_blocks(raw_dirent_table);
        free(inode);
        return NULL;
    }

    size_t block_no = 0;
    size_t padded_name_len = strlen(name) % 4 == 0 ? strlen(name) : strlen(name) + 4 - (strlen(name) % 4);
    size_t new_dirent_size = sizeof(struct raw_dirent) + padded_name_len;
    struct raw_dirent *dirent = raw_dirent_table;
    for (;;) {
        /* We are at the last dirent */
        if (((uintptr_t) EXT2_NEXT_DIRENT(dirent)) - ((uintptr_t) raw_dirent_table) + (block_no * inode->super_block->block_size) >= parent->size) {
            size_t dirent_actual_size = sizeof(struct raw_dirent) + (dirent->name_length % 4 == 0 ? dirent->name_length : dirent->name_length + 4 - (dirent->name_length % 4));
            
            if (inode->super_block->block_size - ((uintptr_t) dirent + dirent_actual_size - (uintptr_t) raw_dirent_table) >= new_dirent_size) {
                dirent->size = dirent_actual_size;
                dirent = (struct raw_dirent*) ((uintptr_t) dirent + dirent_actual_size);
                break;
            } else {
                /* Need to allocate new block */
                assert(block_no < EXT2_SINGLY_INDIRECT_BLOCK_INDEX);
                block_no++;
                ext2_free_blocks(raw_dirent_table);
                raw_dirent_table = dirent = ext2_allocate_blocks(inode->super_block, 1);

                size_t block_index = ext2_find_open_block(inode->super_block, ext2_get_block_group_from_inode(parent->super_block, parent->index));
                parent_raw_inode->block[block_no] = block_index;
                parent->size += parent->super_block->block_size;
                ext2_set_block_allocated(parent->super_block, block_index);
                ext2_sync_inode(parent);
            }
        }

        dirent = EXT2_NEXT_DIRENT(dirent);

        if ((uintptr_t) dirent >= ((uintptr_t) raw_dirent_table) + inode->super_block->block_size) {
            ext2_free_blocks(raw_dirent_table);
            block_no++;

            /* Can't read the indirect blocks */
            if (block_no >= EXT2_SINGLY_INDIRECT_BLOCK_INDEX) {
                assert(false);
            }

            raw_dirent_table = ext2_allocate_blocks(inode->super_block, 1);
            if (ext2_read_blocks(inode->super_block, raw_dirent_table, parent_raw_inode->block[block_no], 1) != 1) {
                debug_log("Ext2 read error (reading dirents): [ %llu ]\n", parent->index);
                *error = -EIO;
                ext2_free_blocks(raw_dirent_table);
                free(inode);
                return NULL;
            }

            dirent = raw_dirent_table;
        }
    }

    debug_log("Adding inode to dir: [ %u, %s ]\n", index, tparent->name);

    /* We have found the right dirent */
    dirent->ino = index;
    memcpy(dirent->name, name, strlen(name));
    dirent->name_length = strlen(name);
    dirent->size = inode->super_block->block_size - ((uintptr_t) dirent - (uintptr_t) raw_dirent_table);
    dirent->type = S_ISREG(mode) ? EXT2_DIRENT_TYPE_REGULAR : S_ISDIR(mode) ? EXT2_DIRENT_TYPE_DIRECTORY : EXT2_DIRENT_TYPE_SOCKET;
    memset((void*) (((uintptr_t) dirent) + sizeof(struct dirent) + dirent->name_length), 0, inode->super_block->block_size - (((uintptr_t) dirent) + sizeof(struct dirent) + dirent->name_length - (uintptr_t) raw_dirent_table));

    ret = ext2_write_blocks(inode->super_block, raw_dirent_table, parent_raw_inode->block[block_no], 1);
    if (ret != 1) {
        debug_log("Ext2 write error (reading dirents): [ %llu ]\n", parent->index);
        *error = -EIO;
        ext2_free_blocks(raw_dirent_table);
        free(inode);
        return NULL;
    }

    ext2_free_blocks(raw_dirent_table);

    return inode;
}

struct tnode *ext2_lookup(struct inode *inode, const char *name) {
    assert(inode);
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

struct file *ext2_open(struct inode *inode, int flags, int *error) {
    (void) flags;

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
static ssize_t __ext2_read(struct file *file, void *buffer, size_t len) {
    assert(file->flags & FS_FILE);
    assert(len >= 1);

    struct inode *inode = fs_inode_get(file->device, file->inode_idenifier);
    assert(inode);
    assert(inode->private_data);

    if (!(inode->mode & S_IRUSR)) {
        return -EPERM;
    }

    /* Indicate done reading */
    if (file->position >= inode->size) {
        return 0;
    }

    size_t max_can_read = inode->size - file->position;
    len = MIN(len, max_can_read);
    ssize_t len_save = (ssize_t) len;

    size_t file_block_no = file->position / inode->super_block->block_size;
    size_t file_block_no_end = (file->position + len + inode->super_block->block_size - 1) / inode->super_block->block_size;

    uint32_t *indirect_block = NULL;

    while (file_block_no < file_block_no_end) {
        void *block = ext2_allocate_blocks(inode->super_block, 1);
        size_t block_no;

        if (file_block_no < EXT2_SINGLY_INDIRECT_BLOCK_INDEX) {
            block_no = ((struct raw_inode*) inode->private_data)->block[file_block_no];
        } else {
            /* Handle single indirect block */
            if (indirect_block == NULL) {
                indirect_block = ext2_allocate_blocks(inode->super_block, 1);
                ssize_t _ret;
                if ((_ret = ext2_read_blocks(inode->super_block, indirect_block, ((struct raw_inode*) inode->private_data)->block[EXT2_SINGLY_INDIRECT_BLOCK_INDEX], 1)) != 1) {
                    ext2_free_blocks(indirect_block);
                    ext2_free_blocks(block);
                    return _ret;
                }
            }
            size_t real_block_offset = file_block_no - EXT2_SINGLY_INDIRECT_BLOCK_INDEX;

            if (real_block_offset * sizeof(uint32_t) >= (size_t) inode->super_block->block_size) {
                /* Should instead start reading from the doubly indirect block */
                ext2_free_blocks(block);
                break;
            }

            block_no = indirect_block[real_block_offset];
        }

        ssize_t ret = ext2_read_blocks(inode->super_block, block, block_no, 1);
        if (ret != 1) {
            return ret;
        }

        size_t buffer_offset = file->position % inode->super_block->block_size;
        size_t to_read = MIN(inode->super_block->block_size - buffer_offset, len);

        memcpy(buffer, (void*) (((uintptr_t) block) + buffer_offset), to_read);
        file->position += to_read;
        len -= to_read;

        ext2_free_blocks(block);
        file_block_no++;
        buffer = (void*) (((uintptr_t) buffer) + inode->super_block->block_size);
    }

    if (indirect_block) {
        ext2_free_blocks(indirect_block);
    }

    return len_save;
}

static ssize_t __ext2_write(struct file *file, const void *buffer, size_t len) {
    assert(file->flags & FS_FILE);

    debug_log("Writing file: [ %lu, %lu ]\n", file->position, len);

    struct inode *inode = fs_inode_get(file->device, file->inode_idenifier);
    struct raw_inode *raw_inode = inode->private_data;
    ssize_t ret = 0;
    inode->size = file->position + len;

    if (!(raw_inode->mode & S_IWUSR)) {
        return -EPERM;
    }

    size_t len_save = len;
    size_t file_block_no = file->position / inode->super_block->block_size;
    size_t file_block_no_end = (file->position + len + inode->super_block->block_size - 1) / inode->super_block->block_size;
    size_t buffer_offset = 0;

    uint32_t *indirect_block = NULL;

    while (file_block_no < file_block_no_end) {
        size_t block_no;

        if (file_block_no < EXT2_SINGLY_INDIRECT_BLOCK_INDEX) {
            if (raw_inode->block[file_block_no] == 0) {
                int __ret = ext2_allocate_block_for_inode(inode->super_block, inode, file_block_no);
                if (__ret != 0) {
                    return ret;
                }
            }

            block_no = raw_inode->block[file_block_no];
        } else {
            /* Handle single indirect block */
            if (indirect_block == NULL) {
                indirect_block = ext2_allocate_blocks(inode->super_block, 1);
                ssize_t _ret;
                if ((_ret = ext2_read_blocks(inode->super_block, indirect_block, ((struct raw_inode*) inode->private_data)->block[EXT2_SINGLY_INDIRECT_BLOCK_INDEX], 1)) != 1) {
                    ext2_free_blocks(indirect_block);
                    return _ret;
                }
            }
            size_t real_block_offset = file_block_no - EXT2_SINGLY_INDIRECT_BLOCK_INDEX;

            if (real_block_offset * sizeof(uint32_t) >= (size_t) inode->super_block->block_size) {
                /* Should instead start writing to the doubly indirect block */
                ext2_free_blocks(indirect_block);
                break;
            }

            if (indirect_block[real_block_offset] == 0) {
                int __ret = ext2_allocate_block_for_inode(inode->super_block, inode, file_block_no);
                if (__ret < 0) {
                    ext2_free_blocks(indirect_block);
                    return __ret;
                }
            }

            block_no = indirect_block[real_block_offset];
        }

        char *buf = ext2_allocate_blocks(inode->super_block, 1);
        if (file->position % inode->super_block->block_size != 0) {
            ret = ext2_read_blocks(inode->super_block, buf, block_no, 1);
            if (ret != 1) {
                return (int) ret;
            }
        }

        size_t to_write = MIN(len, inode->super_block->block_size - (file->position % inode->super_block->block_size));
        memcpy(buf + (file->position % inode->super_block->block_size), (const void*) (((char*) buffer) + buffer_offset), to_write);
        memset(buf + (file->position % inode->super_block->block_size) + to_write, 0, inode->super_block->block_size - (to_write + (file->position % inode->super_block->block_size)));
        ret = ext2_write_blocks(inode->super_block, buf, block_no, 1);
        if (ret != 1) {
            ext2_free_blocks(buf);
            return -EIO;
        }

        ext2_free_blocks(buf);

        file_block_no++;
        len -= to_write;
        buffer_offset += to_write;
        file->position += to_write;
    }

    if (indirect_block) {
        ext2_free_blocks(indirect_block);
    }

    ret = ext2_sync_inode(inode);
    if (ret != 0) {
        return ret;
    }

    return (ssize_t) len_save;
}

ssize_t ext2_read(struct file *file, void *buffer, size_t len) {
    struct inode *inode = fs_inode_get(file->device, file->inode_idenifier);
    spin_lock(&inode->lock);

    ssize_t ret = __ext2_read(file, buffer, len);

    spin_unlock(&inode->lock);
    return ret;
}

ssize_t ext2_write(struct file *file, const void *buffer, size_t len) {
    struct inode *inode = fs_inode_get(file->device, file->inode_idenifier);
    spin_lock(&inode->lock);

    ssize_t ret = __ext2_write(file, buffer, len);

    spin_unlock(&inode->lock);
    return ret;
}

int ext2_stat(struct inode *inode, struct stat *stat_struct) {
    if (!inode->private_data) {
        ext2_update_inode(inode, false);
    }

    struct raw_inode *raw_inode = inode->private_data;

    stat_struct->st_size = inode->size;
    stat_struct->st_blocks = (inode->size / inode->super_block->block_size - 1) + 1;
    stat_struct->st_blksize = inode->super_block->block_size;
    stat_struct->st_ino = inode->index;
    stat_struct->st_dev = inode->device;
    stat_struct->st_mode = inode->mode;
    stat_struct->st_rdev = 0;
    stat_struct->st_nlink = raw_inode->link_count;

    return 0;
}

struct inode *ext2_mkdir(struct tnode *tparent, const char *name, mode_t mode, int *error) {
    if (!(tparent->inode->mode & S_IWUSR)) {
        *error = -EPERM;
        return NULL;
    }

    struct inode *inode = ext2_create(tparent, name, mode, error);
    if (inode == NULL) {
        return NULL;
    }

    struct ext2_block_group *group = ext2_get_block_group(inode->super_block, ext2_get_block_group_from_inode(inode->super_block, inode->index));
    group->blk_desc->num_directories++;
    *error = ext2_sync_block_group(inode->super_block, group->index);
    if (*error != 0) {
        free(inode);
        return NULL;
    }

    struct raw_inode *parent_raw_inode = tparent->inode->private_data;
    parent_raw_inode->link_count++;
    *error = ext2_sync_inode(tparent->inode);
    if (*error != 0) {
        free(inode);
        return NULL;
    }

    struct raw_inode *raw_inode = inode->private_data;
    struct raw_dirent *dirents_start = ext2_allocate_blocks(inode->super_block, 1);
    struct raw_dirent *dirent = dirents_start;

    dirent->ino = (uint32_t) inode->index;
    strcpy(dirent->name, ".");
    dirent->name_length = 1;
    dirent->size = sizeof(struct raw_dirent) + 4;
    dirent->type = EXT2_DIRENT_TYPE_DIRECTORY;

    dirent = EXT2_NEXT_DIRENT(dirent);
    dirent->ino = (uint32_t) tparent->inode->index;
    strcpy(dirent->name, "..");
    dirent->name_length = 2;
    dirent->size = inode->super_block->block_size - (sizeof(struct raw_dirent) + 4);
    dirent->type = EXT2_DIRENT_TYPE_DIRECTORY;

    if (raw_inode->block[0] == 0) {
        *error = ext2_allocate_block_for_inode(inode->super_block, inode, 0);
        if (*error != 0) {
            ext2_free_blocks(dirents_start);
            free(inode);
            return NULL;
        }

        inode->size = inode->super_block->block_size;
        *error = ext2_sync_inode(inode);
        if (*error != 0) {
            ext2_free_blocks(dirents_start);
            free(inode);
            return NULL;
        }
    }

    ssize_t ret = ext2_write_blocks(inode->super_block, dirents_start, raw_inode->block[0], 1);
    if (ret != 1) {
        ext2_free_blocks(dirents_start);
        free(inode);
        *error = (int) ret;
        return NULL;
    }

    ext2_free_blocks(dirents_start);
    return inode;
}

int ext2_unlink(struct tnode *tnode) {
    struct inode *inode = tnode->inode;

    if (inode->private_data == NULL) {
        ext2_update_inode(inode, false);
    }

    struct raw_inode *raw_inode = inode->private_data;
    assert(raw_inode);

    if (!(raw_inode->mode & S_IWUSR)) {
        return -EPERM;
    }

    struct inode *parent = inode->parent->inode;
    struct raw_inode *parent_raw_inode = parent->private_data;
    assert(parent != inode);
    assert(parent_raw_inode != NULL);

    struct raw_dirent *raw_dirent_table = ext2_allocate_blocks(inode->super_block, 1);
    ssize_t ret = ext2_read_blocks(inode->super_block, raw_dirent_table, parent_raw_inode->block[0], 1);
    if (ret != 1) {
        /* We're screwed at this point */
        debug_log("Ext2 read error (reading dirents): [ %llu ]\n", parent->index);
        ext2_free_blocks(raw_dirent_table);
        return -EIO;
    }

    size_t block_no = 0;
    struct raw_dirent *dirent = raw_dirent_table;
    for (;;) {
        /* Found dirent to delete */
        if (dirent->ino == inode->index) {
            break;
        }

        /* Assert we are at the last dirent */
        assert(!(((uintptr_t) EXT2_NEXT_DIRENT(dirent)) - ((uintptr_t) raw_dirent_table) + (block_no * inode->super_block->block_size) >= parent->size));

        dirent = EXT2_NEXT_DIRENT(dirent);

        if ((uintptr_t) dirent >= ((uintptr_t) raw_dirent_table) + inode->super_block->block_size) {
            ext2_free_blocks(raw_dirent_table);
            block_no++;

            /* Can't read the indirect blocks */
            if (block_no >= EXT2_SINGLY_INDIRECT_BLOCK_INDEX) {
                assert(false);
            }

            raw_dirent_table = ext2_allocate_blocks(inode->super_block, 1);
            if (ext2_read_blocks(inode->super_block, raw_dirent_table, parent_raw_inode->block[block_no], 1) != 1) {
                debug_log("Ext2 read error (reading dirents): [ %llu ]\n", parent->index);
                ext2_free_blocks(raw_dirent_table);
                return -EIO;
            }

            dirent = raw_dirent_table;
        }
    }

    debug_log("Removing inode to from: [ %llu, %s ]\n", inode->index, inode->parent->name);

    /* We have found the right dirent, now delete it */
    uint16_t size_save = dirent->size;
    memset(dirent, 0, size_save);
    dirent->size = size_save;

    ret = ext2_write_blocks(inode->super_block, raw_dirent_table, parent_raw_inode->block[block_no], 1);
    if (ret != 1) {
        debug_log("Ext2 write error (dirents): [ %llu ]\n", parent->index);
        ext2_free_blocks(raw_dirent_table);
        return -EIO;
    }

    ext2_free_blocks(raw_dirent_table);

    raw_inode->link_count--;
    if (raw_inode->link_count <= 0) {
        debug_log("Destroying raw ext2 inode: [ %llu ]\n", inode->index);

        /* Actually free inode from disk */
        ext2_free_inode(inode->super_block, inode->index, false);

        /* Free all of the blocks the inode used */
        size_t num_blocks = 0;
        uint32_t *indirect_block = NULL;
        for (;;) {
            size_t block_no;

            if (num_blocks < EXT2_SINGLY_INDIRECT_BLOCK_INDEX) {
                block_no = ((struct raw_inode*) inode->private_data)->block[num_blocks];
            } else {
                /* Handle single indirect block */
                if (indirect_block == NULL) {
                    indirect_block = ext2_allocate_blocks(inode->super_block, 1);
                    ssize_t _ret;
                    debug_log("Indirect block: [ %u ]\n", ((struct raw_inode*) inode->private_data)->block[EXT2_SINGLY_INDIRECT_BLOCK_INDEX]);
                    if ((_ret = ext2_read_blocks(inode->super_block, indirect_block, ((struct raw_inode*) inode->private_data)->block[EXT2_SINGLY_INDIRECT_BLOCK_INDEX], 1)) != 1) {
                        ext2_free_blocks(indirect_block);
                        return _ret;
                    }

                    block_no = ((struct raw_inode*) inode->private_data)->block[EXT2_SINGLY_INDIRECT_BLOCK_INDEX];
                } else  {
                    size_t real_block_offset = num_blocks - EXT2_SINGLY_INDIRECT_BLOCK_INDEX - 1;

                    if (real_block_offset * sizeof(uint32_t) >= (size_t) inode->super_block->block_size) {
                        /* Should instead start reading from the doubly indirect block */
                        break;
                    }

                    block_no = indirect_block[real_block_offset];
                }
            }

            if (block_no == 0) {
                break;
            }

            debug_log("Freeing block: [ %lu ]\n", block_no);
            int ret = ext2_free_block(inode->super_block, block_no, false);
            if (ret != 0) {
                return ret;
            }

            num_blocks++;
        }

        if (indirect_block != NULL) {
            ext2_free_blocks(indirect_block);
        }

        /* Update bookkepping fields */
        struct ext2_block_group *group = ext2_get_block_group(inode->super_block, ext2_get_block_group_from_inode(inode->super_block, inode->index));
        group->blk_desc->num_unallocated_blocks += num_blocks;
        group->blk_desc->num_unallocated_inodes++;
        if (inode->flags & FS_DIR) {
            group->blk_desc->num_directories--;
        }
        int ret = ext2_sync_block_group(inode->super_block, group->index);
        if (ret != 0) {
            return ret;
        }

        struct ext2_sb_data *sb_data = inode->super_block->private_data;
        sb_data->sb->num_unallocated_blocks += num_blocks;
        sb_data->sb->num_unallocated_inodes++;

        ret = ext2_sync_super_block(inode->super_block);
        if (ret != 0) {
            return ret;
        }

        /* Drop our reference to virtual inode so vfs deletes it once it has no open files */
        inode->ref_count--;
        return 0;
    }

    /* Sync new link_count to disk */
    return ext2_sync_inode(inode);
}

int ext2_rmdir(struct tnode *tnode) {
    assert(get_tnode_list_length(tnode->inode->tnode_list) == 2);
    assert(find_tnode(tnode->inode->tnode_list, "."));
    assert(find_tnode(tnode->inode->tnode_list, ".."));

    /* Can't delete reserved inodes */
    if (tnode->inode->index <= ((struct ext2_sb_data*) tnode->inode->super_block->private_data)->sb->first_non_reserved_inode) {
        return -EINVAL;
    }

    struct raw_inode *raw_inode = tnode->inode->private_data;
    struct inode *parent = tnode->inode->parent->inode;
    struct raw_inode *parent_raw_inode = parent->private_data;

    /* Drop .. reference */
    spin_lock(&parent->lock);
    parent_raw_inode->link_count--;
    assert(parent_raw_inode->link_count > 0);

    int ret = ext2_sync_inode(parent);
    if (ret != 0) {
        spin_unlock(&parent->lock);
        return ret;
    }
    spin_unlock(&parent->lock);

    /* Drop . reference */
    raw_inode->link_count--;
    return ext2_unlink(tnode);
}

int ext2_chmod(struct inode *inode, mode_t mode) {
    if (!inode->private_data) {
        ext2_update_inode(inode, true);
    }

    inode->mode = mode;
    return ext2_sync_inode(inode);
}

struct tnode *ext2_mount(struct file_system *current_fs, char *device_path) {
    int error = 0;
    struct file *dev_file = fs_open(device_path, O_RDWR, &error);
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
    data->block_group_map = hash_create_hash_map(block_group_hash, block_group_equals, block_group_key);
    data->block_map = hash_create_hash_map(block_hash, block_equals, block_key);
    data->num_blocks_cached = 0;

    struct ext2_raw_super_block *raw_super_block = ext2_allocate_blocks(super_block, 1);
    if (ext2_read_blocks(super_block, raw_super_block, EXT2_SUPER_BLOCK_OFFSET / EXT2_SUPER_BLOCK_SIZE, 1) != 1) {
        debug_log("Ext2 Read Error: [ Super Block ]\n");
        ext2_free_blocks(raw_super_block);
        return NULL;
    }

    debug_log("Ext2 Num Inodes in Block Group: [ %u ]\n", raw_super_block->num_inodes_in_block_group);
    debug_log("Ext2 Num Blocks: [ %#.8X ]\n", raw_super_block->num_blocks);
    debug_log("Ext2 Num Inodes: [ %#.8X ]\n", raw_super_block->num_inodes);
    debug_log("Ext2 Num Inodes in Group: [ %u ]\n", raw_super_block->num_inodes_in_block_group);
    debug_log("Ext2 Num Blocks in Group: [ %u ]\n", raw_super_block->num_blocks_in_block_group);

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
    root->ref_count = 1;
    root->size = 0;
    root->super_block = super_block;
    root->tnode_list = NULL;

    current_fs->super_block = super_block;

    return t_root;
}

void init_ext2() {
    load_fs(&fs);
}