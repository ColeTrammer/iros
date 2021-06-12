#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/param.h>

#include <kernel/fs/cached_dirent.h>
#include <kernel/fs/dev.h>
#include <kernel/fs/disk_sync.h>
#include <kernel/fs/ext2.h>
#include <kernel/fs/file.h>
#include <kernel/fs/file_system.h>
#include <kernel/fs/inode.h>
#include <kernel/fs/super_block.h>
#include <kernel/fs/vfs.h>
#include <kernel/hal/output.h>
#include <kernel/hal/processor.h>
#include <kernel/mem/inode_vm_object.h>
#include <kernel/mem/page.h>
#include <kernel/mem/phys_page.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/proc/task.h>
#include <kernel/time/clock.h>
#include <kernel/util/init.h>
#include <kernel/util/spinlock.h>

// #define EXT2_DEBUG

static struct block_device_id ext2_fs_ids[] = {
    {
        .type = BLOCK_ID_TYPE_MBR,
        .mbr_id = 0x83,
    },
    {
        .type = BLOCK_ID_TYPE_UUID,
        .uuid = { .raw = { 0xAF, 0x3D, 0xC6, 0x0F, 0x83, 0x84, 0x72, 0x47, 0x8E, 0x79, 0x3D, 0x69, 0xD8, 0x47, 0x7D, 0xE4 } },
    },
};

static struct file_system fs = {
    .name = "ext2",
    .mount = &ext2_mount,
    .umount = &ext2_umount,
    .determine_fsid = &ext2_determine_fsid,
    .id_table = ext2_fs_ids,
    .id_count = sizeof(ext2_fs_ids) / sizeof(ext2_fs_ids[0]),
};

static struct super_block_operations s_op = {
    .rename = &ext2_rename,
    .sync = &ext2_sync_super_block,
};

static struct inode_operations ext2_i_op = {
    .lookup = &ext2_lookup,
    .open = &ext2_open,
    .stat = &ext2_stat,
    .unlink = &ext2_unlink,
    .chmod = &ext2_chmod,
    .chown = &ext2_chown,
    .mmap = &fs_default_mmap,
    .read_all = &ext2_read_all,
    .utimes = &ext2_utimes,
    .read = &ext2_iread,
    .on_inode_destruction = &ext2_on_inode_destruction,
    .truncate = &ext2_truncate,
    .sync = &ext2_sync_inode,
};

static struct inode_operations ext2_dir_i_op = {
    .lookup = &ext2_lookup,
    .open = &ext2_open,
    .stat = &ext2_stat,
    .mkdir = &ext2_mkdir,
    .mknod = &ext2_mknod,
    .rmdir = &ext2_rmdir,
    .chmod = &ext2_chmod,
    .chown = &ext2_chown,
    .symlink = &ext2_symlink,
    .link = &ext2_link,
    .utimes = &ext2_utimes,
    .on_inode_destruction = &ext2_on_inode_destruction,
    .sync = &ext2_sync_inode,
};

static struct file_operations ext2_f_op = {
    .read = &ext2_read,
    .write = &ext2_write,
    .poll = inode_poll,
    .poll_finish = inode_poll_finish,
};

static struct file_operations ext2_dir_f_op = {
    .poll = inode_poll,
    .poll_finish = inode_poll_finish,
};

/* Allocate space to read blocks (eventually should probably not use malloc and instead another mechanism) */
static void *ext2_allocate_blocks(struct super_block *sb, blkcnt_t num_blocks) {
    void *ret = malloc(num_blocks * sb->block_size);
    assert(ret);
    return ret;
}

/* Reads blocks at a given offset into the buffer */
static ssize_t __ext2_read_blocks(struct super_block *sb, void *buffer, uint32_t block_offset, blkcnt_t num_blocks) {
    if (sb->device->ops->read(sb->device, sb->block_size * block_offset, buffer, num_blocks * sb->block_size, false) !=
        num_blocks * sb->block_size) {
        return -EIO;
    }

    return num_blocks;
}

static ssize_t ext2_read_blocks(struct super_block *sb, void *buffer, uint32_t block_offset, blkcnt_t num_blocks) {
    return __ext2_read_blocks(sb, buffer, block_offset, num_blocks);
}

/* Writes to blocks at a given offset from buffer */
static ssize_t ext2_write_blocks(struct super_block *sb, const void *buffer, uint32_t block_offset, blkcnt_t num_blocks) {
    if (sb->device->ops->write(sb->device, sb->block_size * block_offset, buffer, num_blocks * sb->block_size, false) !=
        num_blocks * sb->block_size) {
        return -EIO;
    }

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

static int init_block_usage_bitmap(struct super_block *sb, struct ext2_block_group *group) {
    struct ext2_sb_data *data = sb->private_data;
    ssize_t num_blocks = (data->sb->num_blocks_in_block_group + sb->block_size - 1) / sb->block_size;

    void *blocks = ext2_allocate_blocks(sb, num_blocks);
    if (ext2_read_blocks(sb, blocks, group->blk_desc->block_usage_bitmap_block_address, num_blocks) != num_blocks) {
        debug_log("Ext2 Read block usage bitmap failed: [ %lu ]\n", group->index);
        ext2_free_blocks(blocks);
        return -EIO;
    }
    init_owned_bitset(&group->block_bitset, blocks, num_blocks * sb->block_size, data->sb->num_blocks_in_block_group);
    return 0;
}

static int init_inode_usage_bitmap(struct super_block *sb, struct ext2_block_group *group) {
    struct ext2_sb_data *data = sb->private_data;
    ssize_t num_blocks = (data->sb->num_inodes_in_block_group + sb->block_size - 1) / sb->block_size;

    void *blocks = ext2_allocate_blocks(sb, num_blocks);
    if (ext2_read_blocks(sb, blocks, group->blk_desc->inode_usage_bitmap_block_address, num_blocks) != num_blocks) {
        debug_log("Ext2 Read inode usage bitmap failed: [ %lu ]\n", group->index);
        ext2_free_blocks(blocks);
        return -EIO;
    }
    init_owned_bitset(&group->inode_bitset, blocks, num_blocks * sb->block_size, data->sb->num_inodes_in_block_group);
    return 0;
}

/* Gets block group object for a given block group index */
static struct ext2_block_group *ext2_get_block_group(struct super_block *super_block, size_t index) {
    struct ext2_sb_data *data = super_block->private_data;
    struct ext2_block_group *group = &data->block_groups[index];
    if (!group->initialized) {
        group->index = index;
        group->blk_desc = data->blk_desc_table + index;
        group->initialized = true;

        assert(init_block_usage_bitmap(super_block, group) == 0);
        assert(init_inode_usage_bitmap(super_block, group) == 0);
    }

    return group;
}

/* Syncs block group specified by blk_grp_index */
static int ext2_sync_block_group(struct super_block *sb, uint32_t blk_grp_index) {
    struct ext2_sb_data *data = sb->private_data;
    size_t block_off = blk_grp_index * sizeof(struct raw_block_group_descriptor) / sb->block_size;
    size_t raw_off = block_off * sb->block_size;

    ssize_t ret = ext2_write_blocks(sb, (void *) (((uintptr_t)(data->blk_desc_table)) + raw_off), 2 + block_off, 1);

    if (ret != 1) {
        return (int) ret;
    }

    return 0;
}

static void ext2_sync_raw_super_block_with_virtual_super_block(struct super_block *sb) {
    struct ext2_sb_data *data = sb->private_data;

    sb->num_blocks = data->sb->num_blocks;
    sb->free_blocks = data->sb->num_unallocated_blocks;
    sb->available_blocks = sb->free_blocks - data->sb->num_reserved_blocks;

    sb->num_inodes = data->sb->num_inodes;
    sb->free_inodes = data->sb->num_unallocated_inodes;
    sb->available_inodes = sb->free_inodes - data->sb->first_non_reserved_inode;
}

/* Syncs super_block to disk */
int ext2_sync_super_block(struct super_block *sb) {
    struct ext2_sb_data *data = sb->private_data;

    // Sync to os super_block structure, since we write to the
    // actual raw_super_block when accounting fields are updated.
    ext2_sync_raw_super_block_with_virtual_super_block(sb);

    int ret = sb->device->ops->read(sb->device, EXT2_SUPER_BLOCK_OFFSET, data->sb, EXT2_SUPER_BLOCK_SIZE, false);

    return ret == EXT2_SUPER_BLOCK_SIZE ? 0 : ret;
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
    struct ext2_block_group *block_group = ext2_get_block_group(super_block, index / data->sb->num_blocks_in_block_group);
    size_t rel_offset = index % data->sb->num_blocks_in_block_group;

    bitset_set_bit(&block_group->block_bitset, rel_offset);

    size_t block_off = (rel_offset / 8) * sizeof(uint64_t) / super_block->block_size;
    ssize_t ret =
        ext2_write_blocks(super_block, bitset_data(&block_group->block_bitset) + (block_off * super_block->block_size / sizeof(uint64_t)),
                          ((struct ext2_sb_data *) super_block->private_data)
                                  ->blk_desc_table[index / data->sb->num_blocks_in_block_group]
                                  .block_usage_bitmap_block_address +
                              block_off,
                          1);
    if (ret != 1) {
        return (int) ret;
    }

    block_group->blk_desc->num_unallocated_blocks--;
    ret = ext2_sync_block_group(super_block, index / data->sb->num_blocks_in_block_group);

    if (ret != 0) {
        return (int) ret;
    }

    data->sb->num_unallocated_blocks--;
    fs_mark_super_block_as_dirty(super_block);
    return 0;
}

/* Frees a block from block bitmap */
static int ext2_free_block(struct super_block *super_block, uint32_t index, bool sync_bookkeeping_fields) {
    index--;
    struct ext2_sb_data *data = super_block->private_data;
    struct ext2_block_group *block_group = ext2_get_block_group(super_block, index / data->sb->num_blocks_in_block_group);
    size_t rel_offset = index % data->sb->num_blocks_in_block_group;

    bitset_clear_bit(&block_group->block_bitset, rel_offset);

    size_t block_off = (rel_offset / 8) * sizeof(uint64_t) / super_block->block_size;
    ssize_t ret =
        ext2_write_blocks(super_block, bitset_data(&block_group->block_bitset) + (block_off * super_block->block_size / sizeof(uint64_t)),
                          ((struct ext2_sb_data *) super_block->private_data)
                                  ->blk_desc_table[index / data->sb->num_blocks_in_block_group]
                                  .block_usage_bitmap_block_address +
                              block_off,
                          1);
    if (ret != 1) {
        return (int) ret;
    }

    if (sync_bookkeeping_fields) {
        block_group->blk_desc->num_unallocated_blocks--;
        ret = ext2_sync_block_group(super_block, index / data->sb->num_blocks_in_block_group);

        if (ret != 0) {
            return (int) ret;
        }

        data->sb->num_unallocated_blocks--;
        fs_mark_super_block_as_dirty(super_block);
    }

    return 0;
}

/* Find an open block (using usage bitmap), starting with the one specified by blk_grp_index */
static uint32_t ext2_find_open_block(struct super_block *sb, size_t blk_grp_index) {
    struct ext2_sb_data *data = sb->private_data;
    struct ext2_block_group *group = ext2_get_block_group(sb, blk_grp_index);

    size_t block_index;
    int ret = bitset_find_first_free_bit(&group->block_bitset, &block_index);
    if (ret < 0) {
        size_t save_blk_grp_index = blk_grp_index;
        for (blk_grp_index = 0; blk_grp_index < data->num_block_groups; blk_grp_index++) {
            if (blk_grp_index == save_blk_grp_index) {
                continue;
            }

            ret = bitset_find_first_free_bit(&group->block_bitset, &block_index);
            if (ret >= 0) {
                break;
            }
        }
    }

    if (ret < 0) {
        return 0;
    }

    block_index += data->sb->num_blocks_in_block_group * blk_grp_index + 1;
#ifdef EXT2_DEBUG
    debug_log("Allocated block index: [ %ld ]\n", block_index);
#endif /* EXT2_DEBUG */
    return block_index;
}

/* Gets inode usage bitmap for a given block group index */
static int ext2_set_inode_allocated(struct super_block *super_block, uint32_t index) {
    struct ext2_sb_data *data = super_block->private_data;
    struct ext2_block_group *block_group = ext2_get_block_group(super_block, index / data->sb->num_blocks_in_block_group);
    size_t rel_offset = ext2_get_inode_table_index(super_block, index);

    bitset_set_bit(&block_group->inode_bitset, rel_offset);

    size_t block_off = (rel_offset / 8) * sizeof(uint64_t) / super_block->block_size;
    ssize_t ret =
        ext2_write_blocks(super_block, bitset_data(&block_group->inode_bitset) + (block_off * super_block->block_size / sizeof(uint64_t)),
                          ((struct ext2_sb_data *) super_block->private_data)
                                  ->blk_desc_table[ext2_get_block_group_from_inode(super_block, index)]
                                  .inode_usage_bitmap_block_address +
                              block_off,
                          1);
    if (ret != 1) {
        return (int) ret;
    }

    block_group->blk_desc->num_unallocated_inodes--;
    ret = ext2_sync_block_group(super_block, ext2_get_block_group_from_inode(super_block, index));

    if (ret != 0) {
        return (int) ret;
    }

    struct ext2_sb_data *sb_data = super_block->private_data;
    sb_data->sb->num_unallocated_inodes--;
    fs_mark_super_block_as_dirty(super_block);
    return 0;
}

/* Frees an inode index in the inode bitmap */
static int ext2_free_inode(struct super_block *super_block, uint32_t index, bool sync_bookkeeping_fields) {
    struct ext2_sb_data *data = super_block->private_data;
    struct ext2_block_group *block_group = ext2_get_block_group(super_block, index / data->sb->num_blocks_in_block_group);
    size_t rel_offset = ext2_get_inode_table_index(super_block, index);

    bitset_clear_bit(&block_group->inode_bitset, rel_offset);

    size_t block_off = (rel_offset / 8) * sizeof(uint64_t) / super_block->block_size;
    ssize_t ret =
        ext2_write_blocks(super_block, bitset_data(&block_group->inode_bitset) + (block_off * super_block->block_size / sizeof(uint64_t)),
                          ((struct ext2_sb_data *) super_block->private_data)
                                  ->blk_desc_table[ext2_get_block_group_from_inode(super_block, index)]
                                  .inode_usage_bitmap_block_address +
                              block_off,
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
        fs_mark_super_block_as_dirty(super_block);
    }

    return 0;
}

/* Find an open inode (using usage bitmap), starting with the one specified by blk_grp_index */
static uint32_t ext2_find_open_inode(struct super_block *sb, size_t blk_grp_index) {
    struct ext2_sb_data *data = sb->private_data;
    struct ext2_block_group *group = ext2_get_block_group(sb, blk_grp_index);

    size_t inode_index;
    int ret = bitset_find_first_free_bit(&group->inode_bitset, &inode_index);
    if (ret < 0) {
        size_t save_blk_grp_index = blk_grp_index;
        for (blk_grp_index = 0; blk_grp_index < data->num_block_groups; blk_grp_index++) {
            if (blk_grp_index == save_blk_grp_index) {
                continue;
            }

            ret = bitset_find_first_free_bit(&group->inode_bitset, &inode_index);
            if (ret >= 0) {
                break;
            }
        }
    }

    if (ret < 0) {
        return 0;
    }

    inode_index += data->sb->num_inodes_in_block_group * blk_grp_index + 1;
#ifdef EXT2_DEBUG
    debug_log("Allocated inode index: [ %ld ]\n", inode_index);
#endif /* EXT2_DEBUG */
    return (uint32_t) inode_index;
}

void ext2_init_block_iterator(struct ext2_block_iterator *iter, struct inode *inode, bool write_mode) {
    memset(iter, 0, sizeof(*iter));
    iter->limits[0] = EXT2_SINGLY_INDIRECT_BLOCK_INDEX;
    iter->inode = inode;
    iter->raw_inode = inode->private_data;
    iter->super_block = inode->super_block;
    iter->write_mode = write_mode;
}

void ext2_kill_block_iterator(struct ext2_block_iterator *iter) {
    for (uint32_t i = 0; i < 3; i++) {
        if (iter->indirect[i]) {
            ext2_free_blocks(iter->indirect[i]);
        }
    }
    memset(iter, 0, sizeof(*iter));
}

int ext2_block_iterator_set_byte_offset(struct ext2_block_iterator *, off_t) {
    return -EIO;
}

int ext2_block_iterator_next(struct ext2_block_iterator *iter, uint32_t *block_out) {
restart:
    for (; iter->write_mode || iter->byte_offset < iter->inode->size;) {
        for (int level = iter->level; level >= 0; level--) {
            bool finished_level = true;
            for (int i = 0; i <= level; i++) {
                if (iter->indexes[i] < iter->limits[i]) {
                    finished_level = false;
                    break;
                }
            }

            if (finished_level) {
                // This means, for example, the singly indirect block that is part of doubly indirect block has been exhausted.
                if (level < iter->level) {
                    // Since this level's indirect block is done, a new one must be fetched from a block with a higher level of indirection.
                    unsigned int indirect_block = iter->indirect[level + 1][iter->indexes[level + 1]++];
                    if (indirect_block == 0) {
                        if (!iter->write_mode) {
                            return 0;
                        }

                        assert(false);
                    }

                    if (ext2_read_blocks(iter->super_block, iter->indirect[level], indirect_block, 1) < 0) {
                        return -EIO;
                    }
                    iter->indexes[level] = 0;
                    continue;
                }

                // No more levels
                if (level == 3) {
                    return 0;
                }

                // We need to start iterating the next level.
                for (level = !iter->at_indirect ? 0 : level + 1; level < 3; level++) {
                    unsigned int new_indirect_block = iter->raw_inode->block[EXT2_SINGLY_INDIRECT_BLOCK_INDEX + level];
                    if (new_indirect_block == 0) {
                        if (!iter->write_mode) {
                            return 0;
                        }

                        assert(false);
                    }

                    if (!iter->indirect[level]) {
                        iter->indirect[level] = ext2_allocate_blocks(iter->super_block, 1);
                    }

                    if (ext2_read_blocks(iter->super_block, iter->indirect[level], new_indirect_block, 1) < 0) {
                        return -EIO;
                    }

                    iter->level = level;
                    iter->limits[level] = iter->super_block->block_size / sizeof(uint32_t);
                    iter->indexes[level] = 0;
                    iter->at_indirect = 1;
                    goto restart;
                }

                // There were no more indirect blocks.
                return 0;
            }
        }

        uint32_t *block_array = !iter->at_indirect ? iter->raw_inode->block : iter->indirect[0];
        uint32_t current_block = block_array[iter->indexes[0]++];
        iter->byte_offset += iter->super_block->block_size;
        if (current_block == 0) {
            if (!iter->write_mode) {
                return 0;
            }

            assert(false);
        }

        if (current_block != 0) {
            *block_out = current_block;
            return 1;
        }
    }

    return 0;
}

/* Gets the raw inode from index */
static struct raw_inode *ext2_get_raw_inode(struct super_block *sb, uint32_t index) {
    struct ext2_sb_data *sb_data = sb->private_data;

    size_t inode_table_index = ext2_get_inode_table_index(sb, index);
    size_t block_off = (inode_table_index * sb_data->sb->inode_size) / sb->block_size;
    size_t block_group_index = ext2_get_block_group_from_inode(sb, index);

    char *block = ext2_allocate_blocks(sb, 1);
    struct ext2_block_group *group = ext2_get_block_group(sb, block_group_index);
    size_t block_address = group->blk_desc->inode_table_block_address + block_off;
    ssize_t ret = ext2_read_blocks(sb, block, block_address, 1);
    if (ret != 1) {
        debug_log("Error reading inode table: [ %lu, %lu, %lu ]\n", block_group_index, block_address, inode_table_index);
        ext2_free_blocks(block);
        return NULL;
    }

    struct raw_inode *raw_inode = (struct raw_inode *) (block + (inode_table_index * sb_data->sb->inode_size) % sb->block_size);
    struct raw_inode *raw_inode_copy = malloc(sizeof(struct raw_inode));
    memcpy(raw_inode_copy, raw_inode, sb_data->sb->inode_size);

    ext2_free_blocks(block);
    return raw_inode_copy;
}

static void ext2_update_inode(struct inode *inode, bool update_tnodes);

/* Reads dirent entries of an inode */
static void ext2_update_tnode_list(struct inode *inode) {
    if (!inode->private_data) {
        ext2_update_inode(inode, false);
    }

    mutex_lock(&inode->lock);

    if (inode->dirent_cache != NULL) {
        // NOTE: this prevents duplicate tnode's from being created.
        //       should eventually just not add duplicates instead though.
        mutex_unlock(&inode->lock);
        return;
    } else {
        inode->dirent_cache = fs_create_dirent_cache();
    }

    if (inode->size == 0) {
        debug_log("directory inode empty?: [ %llu ]\n", inode->index);
        mutex_unlock(&inode->lock);
        return;
    }

    struct ext2_block_iterator iter;
    ext2_init_block_iterator(&iter, inode, false);

    void *raw_dirent_table = ext2_allocate_blocks(inode->super_block, 1);
    uint32_t block_index;
    int ret;
    while ((ret = ext2_block_iterator_next(&iter, &block_index)) == 1) {
        if (ext2_read_blocks(inode->super_block, raw_dirent_table, block_index, 1) < 0) {
            goto finish_update_tnode_list;
        }

        uint32_t byte_offset = 0;
        for (struct raw_dirent *dirent = raw_dirent_table; byte_offset < inode->super_block->block_size;
             dirent = raw_dirent_table + byte_offset) {
            if (dirent->ino == 0 || dirent->type == EXT2_DIRENT_TYPE_UNKNOWN) {
                goto next_dirent;
            }

            if ((dirent->name[0] == '.' && dirent->name_length == 1) ||
                (dirent->name[0] == '.' && dirent->name[1] == '.' && dirent->name[2] == '\0')) {
                goto next_dirent;
            }

            struct inode *inode_to_add;
            inode_to_add = calloc(1, sizeof(struct inode));
            inode_to_add->fsid = inode->fsid;
            inode_to_add->index = dirent->ino;
            inode_to_add->i_op = dirent->type == EXT2_DIRENT_TYPE_DIRECTORY ? &ext2_dir_i_op : &ext2_i_op;
            inode_to_add->super_block = inode->super_block;
            inode_to_add->flags = dirent->type == EXT2_DIRENT_TYPE_REGULAR         ? FS_FILE
                                  : dirent->type == EXT2_DIRENT_TYPE_SOCKET        ? FS_SOCKET
                                  : dirent->type == EXT2_DIRENT_TYPE_SYMBOLIC_LINK ? FS_LINK
                                  : (dirent->type == EXT2_DIRENT_TYPE_BLOCK || dirent->type == EXT2_DIRENT_TYPE_CHARACTER_DEVICE)
                                      ? FS_DEVICE
                                  : dirent->type == EXT2_DIRENT_TYPE_FIFO ? FS_FIFO
                                                                          : FS_DIR;
            inode_to_add->ref_count = 2; // One for the vfs and one for us
            init_file_state(&inode_to_add->file_state, !!inode->size,
                            !!((inode->flags & FS_DIR) | (inode->flags & FS_FILE) | (inode->flags & FS_LINK)));
            init_mutex(&inode_to_add->lock);

            fs_put_dirent_cache(inode->dirent_cache, inode_to_add, dirent->name, dirent->name_length);

        next_dirent:
            byte_offset += dirent->size;
        }
    }

finish_update_tnode_list:
    ext2_kill_block_iterator(&iter);
    mutex_unlock(&inode->lock);
    ext2_free_blocks(raw_dirent_table);
}

/* Reads raw inode info into memory and updates inode */
static void ext2_update_inode(struct inode *inode, bool update_tnodes) {
    assert(inode->private_data == NULL);

    struct raw_inode *raw_inode = ext2_get_raw_inode(inode->super_block, inode->index);
    assert(raw_inode);

    inode->private_data = raw_inode;
    inode->mode = raw_inode->mode;
    inode->uid = raw_inode->uid;
    inode->gid = raw_inode->gid;
    inode->size = raw_inode->size;
    inode->access_time = (struct timespec) { .tv_sec = raw_inode->atime, .tv_nsec = 0 };
    inode->modify_time = (struct timespec) { .tv_sec = raw_inode->mtime, .tv_nsec = 0 };
    inode->change_time = (struct timespec) { .tv_sec = raw_inode->ctime, .tv_nsec = 0 };

    if (update_tnodes && inode->flags & FS_DIR && inode->dirent_cache == NULL) {
        ext2_update_tnode_list(inode);
    }

    if (S_ISBLK(inode->mode) || S_ISCHR(inode->mode)) {
        dev_t device_number = raw_inode->block[0];
        if (!device_number) {
            device_number = raw_inode->block[1];
        }
        device_number &= 0xFFFFFU;
        inode->device_id = device_number;
    }
}

/* Syncs raw_inode to disk */
int ext2_sync_inode(struct inode *inode) {
    struct ext2_sb_data *sb_data = inode->super_block->private_data;

    size_t inode_table_index = ext2_get_inode_table_index(inode->super_block, inode->index);
    size_t block_off = (inode_table_index * sb_data->sb->inode_size) / inode->super_block->block_size;
    size_t block_group_index = ext2_get_block_group_from_inode(inode->super_block, inode->index);

    char *block = ext2_allocate_blocks(inode->super_block, 1);
    struct ext2_block_group *group = ext2_get_block_group(inode->super_block, block_group_index);
    size_t block_address = group->blk_desc->inode_table_block_address + block_off;
    ssize_t ret = ext2_read_blocks(inode->super_block, block, block_address, 1);
    if (ret != 1) {
        debug_log("Error reading inode table: [ %lu, %lu, %lu ]\n", block_group_index, block_address, inode_table_index);
        ext2_free_blocks(block);
        return (int) ret;
    }

    struct raw_inode *raw_inode = inode->private_data;

    raw_inode->size = inode->size;
    raw_inode->mode = inode->mode;
    raw_inode->uid = inode->uid;
    raw_inode->gid = inode->gid;
    raw_inode->atime = inode->access_time.tv_sec;
    raw_inode->mtime = inode->modify_time.tv_sec;
    raw_inode->ctime = inode->change_time.tv_sec;
    /* Sector size should be retrieved from block device */
    blksize_t blk_size = dev_block_size(inode->super_block->device);
    raw_inode->sectors = (inode->size + blk_size - 1) / blk_size;

    memcpy(block + (inode_table_index * sb_data->sb->inode_size) % inode->super_block->block_size, raw_inode, sb_data->sb->inode_size);
    ret = ext2_write_blocks(inode->super_block, block, block_address, 1);

    if (ret != 1) {
        debug_log("Error writing inode table: [ %lu, %lu, %lu ]\n", block_group_index, block_address, inode_table_index);
        ext2_free_blocks(block);
        return (int) ret;
    }

    ext2_free_blocks(block);
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
    raw_inode->uid = inode->uid;
    raw_inode->size = 0;
    raw_inode->atime = inode->access_time.tv_sec;
    raw_inode->ctime = inode->change_time.tv_sec;
    raw_inode->mtime = inode->modify_time.tv_sec;
    raw_inode->dtime = 0;
    raw_inode->gid = inode->gid;
    raw_inode->link_count = S_ISDIR(inode->mode) ? 2 : 1;
    raw_inode->sectors = 0;
    raw_inode->flags = 0;
    raw_inode->os_specific_1 = 0;
    ssize_t to_preallocate = S_ISREG(inode->mode)   ? data->sb->num_blocks_to_preallocate_for_files
                             : S_ISDIR(inode->mode) ? data->sb->num_blocks_to_preallocate_for_directories
                                                    : 0;
    if (to_preallocate != 0) {
        void *zeroes = ext2_allocate_blocks(inode->super_block, 1);
        memset(zeroes, 0, inode->super_block->block_size);
        for (ssize_t i = 0; i < to_preallocate; i++) {
            int ret = ext2_allocate_block_for_inode(inode->super_block, inode, i);
            if (ret != 0) {
                return ret;
            }
        }
        ext2_free_blocks(zeroes);
    }

    memset(raw_inode->block + MIN(EXT2_SINGLY_INDIRECT_BLOCK_INDEX, to_preallocate), 0, (MAX(15 - to_preallocate, 2)) * sizeof(uint32_t));
    if (S_ISCHR(inode->mode) || S_ISBLK(inode->mode)) {
        raw_inode->block[1] = inode->device_id;
    }

    raw_inode->generation = 0;
    raw_inode->file_acl = 0;
    raw_inode->dir_acl = 0;
    raw_inode->faddr = 0;
    memset(raw_inode->os_specific_2, 0, 12 * sizeof(uint8_t));

    fs_mark_inode_as_dirty(inode);
    return 0;
}

struct inode *__ext2_create(struct tnode *tparent, const char *name, mode_t mode, int *error, ino_t inode_id, dev_t device) {
    struct inode *parent = tparent->inode;
    uint32_t index;
    struct inode *inode = NULL;
    if (inode_id != 0) {
        index = (uint32_t) inode_id;
    } else {
        index = ext2_find_open_inode(parent->super_block, ext2_get_block_group_from_inode(parent->super_block, parent->index));
        inode = calloc(1, sizeof(struct inode));
    }
    if (index == 0) {
        *error = -ENOSPC;
        free(inode);
        return NULL;
    }

    if (inode_id == 0) {
        *error = ext2_set_inode_allocated(parent->super_block, index);
        if (*error != 0) {
            free(inode);
            return NULL;
        }

        inode->fsid = parent->fsid;
        inode->flags = fs_mode_to_flags(mode);
        inode->i_op = S_ISDIR(mode) ? &ext2_dir_i_op : &ext2_i_op;
        inode->device_id = device;
        inode->index = index;
        init_mutex(&inode->lock);
        inode->mode = mode;
        inode->uid = get_current_task()->process->euid;
        inode->gid = get_current_task()->process->egid;
        inode->private_data = NULL;
        inode->ref_count = 2; // One for the vfs and one for us
        inode->size = 0;
        inode->super_block = parent->super_block;
        init_file_state(&inode->file_state, !!inode->size, true);
        inode->change_time = inode->modify_time = inode->access_time = time_read_clock(CLOCK_REALTIME);

        *error = ext2_write_inode(inode);
        if (errno != 0) {
            free(inode);
            return NULL;
        }
    }

    if (parent->private_data == NULL) {
        ext2_update_inode(parent, true);
    }

    struct raw_inode *parent_raw_inode = parent->private_data;
    struct raw_dirent *raw_dirent_table = ext2_allocate_blocks(parent->super_block, 1);
    ssize_t ret = ext2_read_blocks(parent->super_block, raw_dirent_table, parent_raw_inode->block[0], 1);
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
        if (((uintptr_t) EXT2_NEXT_DIRENT(dirent)) - ((uintptr_t) raw_dirent_table) + (block_no * parent->super_block->block_size) >=
            parent->size) {
            size_t dirent_actual_size =
                sizeof(struct raw_dirent) +
                (dirent->name_length % 4 == 0 ? dirent->name_length : dirent->name_length + 4 - (dirent->name_length % 4));

            if (parent->super_block->block_size - ((uintptr_t) dirent + dirent_actual_size - (uintptr_t) raw_dirent_table) >=
                new_dirent_size) {
                dirent->size = dirent_actual_size;
                dirent = (struct raw_dirent *) ((uintptr_t) dirent + dirent_actual_size);
                break;
            } else {
                /* Need to allocate new block */
                assert(block_no < EXT2_SINGLY_INDIRECT_BLOCK_INDEX);
                block_no++;
                ext2_free_blocks(raw_dirent_table);
                raw_dirent_table = dirent = ext2_allocate_blocks(parent->super_block, 1);

                size_t block_index =
                    ext2_find_open_block(parent->super_block, ext2_get_block_group_from_inode(parent->super_block, parent->index));
                parent_raw_inode->block[block_no] = block_index;
                parent->size += parent->super_block->block_size;
                ext2_set_block_allocated(parent->super_block, block_index);
                parent->modify_time = time_read_clock(CLOCK_REALTIME);
                fs_mark_inode_as_dirty(parent);
            }
        }

        dirent = EXT2_NEXT_DIRENT(dirent);

        if ((uintptr_t) dirent >= ((uintptr_t) raw_dirent_table) + parent->super_block->block_size) {
            ext2_free_blocks(raw_dirent_table);
            block_no++;

            /* Can't read the indirect blocks */
            if (block_no >= EXT2_SINGLY_INDIRECT_BLOCK_INDEX) {
                assert(false);
            }

            raw_dirent_table = ext2_allocate_blocks(parent->super_block, 1);
            if (ext2_read_blocks(parent->super_block, raw_dirent_table, parent_raw_inode->block[block_no], 1) != 1) {
                debug_log("Ext2 read error (reading dirents): [ %llu ]\n", parent->index);
                *error = -EIO;
                ext2_free_blocks(raw_dirent_table);
                free(inode);
                return NULL;
            }

            dirent = raw_dirent_table;
        }
    }

#ifdef EXT2_DEBUG
    debug_log("Adding inode to dir: [ %u, %s ]\n", index, tparent->name);
#endif /* EXT2_DEBUG */

    /* We have found the right dirent */
    dirent->ino = index;
    memcpy(dirent->name, name, strlen(name));
    dirent->name_length = strlen(name);
    dirent->size = parent->super_block->block_size - ((uintptr_t) dirent - (uintptr_t) raw_dirent_table);
    dirent->type = S_ISREG(mode)    ? EXT2_DIRENT_TYPE_REGULAR
                   : S_ISDIR(mode)  ? EXT2_DIRENT_TYPE_DIRECTORY
                   : S_ISLNK(mode)  ? EXT2_DIRENT_TYPE_SYMBOLIC_LINK
                   : S_ISBLK(mode)  ? EXT2_DIRENT_TYPE_BLOCK
                   : S_ISCHR(mode)  ? EXT2_DIRENT_TYPE_CHARACTER_DEVICE
                   : S_ISFIFO(mode) ? EXT2_DIRENT_TYPE_FIFO
                                    : EXT2_DIRENT_TYPE_SOCKET;
    memset((void *) (((uintptr_t) dirent) + sizeof(struct dirent) + dirent->name_length), 0,
           parent->super_block->block_size -
               (((uintptr_t) dirent) + sizeof(struct dirent) + dirent->name_length - (uintptr_t) raw_dirent_table));

    ret = ext2_write_blocks(parent->super_block, raw_dirent_table, parent_raw_inode->block[block_no], 1);
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

struct inode *ext2_lookup(struct inode *inode, const char *name) {
    assert(inode);

    if (!(inode->flags & FS_DIR)) {
        if (!inode->private_data) {
            ext2_update_inode(inode, false);
        }
        return NULL;
    }

    ext2_update_tnode_list(inode);

    if (name == NULL) {
        return NULL;
    }

    return fs_lookup_in_cache(inode->dirent_cache, name);
}

struct file *ext2_open(struct inode *inode, int flags, int *error) {
    if (!inode->private_data) {
        ext2_update_inode(inode, true);
    }

    *error = 0;
    return fs_create_file(inode, inode->flags, 0, flags, inode->flags & FS_DIR ? &ext2_dir_f_op : &ext2_f_op, NULL);
}

/* Should provide some sort of mechanism for caching these blocks */
static ssize_t __ext2_read(struct inode *inode, off_t offset, void *buffer, size_t len) {
    assert(len >= 1);

    /* Indicate done reading */
    if (offset >= (off_t) inode->size) {
        return 0;
    }

    size_t max_can_read = inode->size - offset;
    len = MIN(len, max_can_read);
    ssize_t len_save = (ssize_t) len;

    // Read directly from blocks if the inode->size < 60
    if ((inode->flags & FS_LINK) && (inode->size < 60)) {
        memcpy(buffer, (char *) ((struct raw_inode *) inode->private_data)->block + offset, len);
        return len;
    }

    size_t file_block_no = offset / inode->super_block->block_size;
    size_t file_block_no_end = (offset + len) / inode->super_block->block_size;

    uint32_t *indirect_block = NULL;
    uint32_t *double_indirect_block = NULL;
    ssize_t double_indirect_block_offset = -1;

    while (file_block_no <= file_block_no_end) {
        void *block = ext2_allocate_blocks(inode->super_block, 1);
        size_t block_no;

        if (file_block_no < EXT2_SINGLY_INDIRECT_BLOCK_INDEX) {
            block_no = ((struct raw_inode *) inode->private_data)->block[file_block_no];
        } else {
            /* Handle single indirect block */
            if (indirect_block == NULL) {
                indirect_block = ext2_allocate_blocks(inode->super_block, 1);
                if (double_indirect_block_offset == -1) {
                    ssize_t _ret;
                    if ((_ret = ext2_read_blocks(inode->super_block, indirect_block,
                                                 ((struct raw_inode *) inode->private_data)->block[EXT2_SINGLY_INDIRECT_BLOCK_INDEX], 1)) !=
                        1) {
                        ext2_free_blocks(indirect_block);
                        ext2_free_blocks(block);
                        return _ret;
                    }
                } else {
                    ssize_t _ret;
                    if ((_ret = ext2_read_blocks(inode->super_block, indirect_block, double_indirect_block[double_indirect_block_offset],
                                                 1)) != 1) {
                        ext2_free_blocks(double_indirect_block);
                        ext2_free_blocks(indirect_block);
                        ext2_free_blocks(block);
                        return _ret;
                    }
                }
            }
            size_t real_block_offset = file_block_no - EXT2_SINGLY_INDIRECT_BLOCK_INDEX;

            if (real_block_offset * sizeof(uint32_t) >=
                (size_t) inode->super_block->block_size * (size_t)(double_indirect_block_offset + 2)) {
                ext2_free_blocks(indirect_block);
                indirect_block = NULL;

                if (double_indirect_block == NULL) {
                    double_indirect_block = ext2_allocate_blocks(inode->super_block, 1);
                    ssize_t _ret;
                    if ((_ret = ext2_read_blocks(inode->super_block, double_indirect_block,
                                                 ((struct raw_inode *) inode->private_data)->block[EXT2_DOUBLY_INDIRECT_BLOCK_INDEX], 1)) !=
                        1) {
                        ext2_free_blocks(double_indirect_block);
                        ext2_free_blocks(block);
                        return _ret;
                    }
                }

                double_indirect_block_offset++;
                ext2_free_blocks(block);
                continue;
            }

            if (double_indirect_block_offset == -1) {
                block_no = indirect_block[real_block_offset];
            } else {
                assert((size_t) double_indirect_block_offset < inode->super_block->block_size / sizeof(uint32_t));
                block_no = indirect_block[real_block_offset - (double_indirect_block_offset + 1) *
                                                                  ((size_t) inode->super_block->block_size) / sizeof(uint32_t)];
            }
        }

        ssize_t ret = ext2_read_blocks(inode->super_block, block, block_no, 1);
        if (ret != 1) {
            return ret;
        }

        size_t buffer_offset = offset % inode->super_block->block_size;
        size_t to_read = MIN(inode->super_block->block_size - buffer_offset, len);

        memcpy(buffer, (void *) (((uintptr_t) block) + buffer_offset), to_read);
        offset += to_read;
        len -= to_read;

        ext2_free_blocks(block);
        file_block_no++;
        buffer = (void *) (((uintptr_t) buffer) + to_read);
    }

    if (indirect_block) {
        ext2_free_blocks(indirect_block);
    }

    if (double_indirect_block) {
        ext2_free_blocks(double_indirect_block);
    }

    return len_save;
}

static ssize_t __ext2_write(struct inode *inode, off_t offset, const void *buffer, size_t len) {
#ifdef EXT2_DEBUG
    debug_log("Writing file: [ %lu, %lu ]\n", offset, len);
#endif /* EXT2_DEBUG */

    struct raw_inode *raw_inode = inode->private_data;
    ssize_t ret = 0;

    size_t len_save = len;
    size_t file_block_no = offset / inode->super_block->block_size;
    size_t file_block_no_end = (offset + len + inode->super_block->block_size - 1) / inode->super_block->block_size;
    size_t buffer_offset = 0;

    uint32_t *indirect_block = NULL;

    while (file_block_no < file_block_no_end) {
        size_t block_no;

        if (file_block_no < EXT2_SINGLY_INDIRECT_BLOCK_INDEX) {
            if (raw_inode->block[file_block_no] == 0) {
                int __ret = ext2_allocate_block_for_inode(inode->super_block, inode, file_block_no);
                if (__ret != 0) {
                    return __ret;
                }
            }

            block_no = raw_inode->block[file_block_no];
        } else {
            /* Handle single indirect block */
            if (indirect_block == NULL) {
                indirect_block = ext2_allocate_blocks(inode->super_block, 1);
                ssize_t _ret;
                if ((_ret = ext2_read_blocks(inode->super_block, indirect_block,
                                             ((struct raw_inode *) inode->private_data)->block[EXT2_SINGLY_INDIRECT_BLOCK_INDEX], 1)) !=
                    1) {
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
        size_t in_block_offset = offset % inode->super_block->block_size;
        size_t to_write = MIN(len, inode->super_block->block_size - in_block_offset);
        bool read_block = ((size_t) offset < inode->size) || (in_block_offset != 0);
        if (read_block) {
            ret = ext2_read_blocks(inode->super_block, buf, block_no, 1);
            if (ret != 1) {
                return (int) ret;
            }
        } else {
            memset(buf, 0, in_block_offset);
            memset(buf + in_block_offset + to_write, 0, inode->super_block->block_size - (to_write + in_block_offset));
        }

        memcpy(buf + in_block_offset, (const void *) (((char *) buffer) + buffer_offset), to_write);
        ret = ext2_write_blocks(inode->super_block, buf, block_no, 1);
        if (ret != 1) {
            ext2_free_blocks(buf);
            return -EIO;
        }

        ext2_free_blocks(buf);

        file_block_no++;
        len -= to_write;
        buffer_offset += to_write;
        offset += to_write;
    }

    if (indirect_block) {
        ext2_free_blocks(indirect_block);
    }

    inode->size = MAX(inode->size, offset + len);
    inode->modify_time = time_read_clock(CLOCK_REALTIME);
    fs_mark_inode_as_dirty(inode);
    return (ssize_t) len_save;
}

int ext2_truncate(struct inode *inode, off_t size) {
    size_t new_size = (size_t) size;
    size_t old_size = inode->size;

    struct raw_inode *raw_inode = inode->private_data;
    assert(raw_inode);

    if (new_size < old_size) {
        size_t num_blocks_freed = 0;
        size_t block_index = 0;

        uint32_t *indirect_block = NULL;
        for (;;) {
            size_t block_no;

            if (block_index < EXT2_SINGLY_INDIRECT_BLOCK_INDEX) {
                block_no = ((struct raw_inode *) inode->private_data)->block[block_index];
            } else {
                if (indirect_block == NULL) {
                    indirect_block = ext2_allocate_blocks(inode->super_block, 1);
                    ssize_t _ret;
#ifdef EXT2_DEBUG
                    debug_log("Indirect block: [ %u ]\n",
                              ((struct raw_inode *) inode->private_data)->block[EXT2_SINGLY_INDIRECT_BLOCK_INDEX]);
#endif /* EXT2_DEBUG */
                    if ((_ret = ext2_read_blocks(inode->super_block, indirect_block,
                                                 ((struct raw_inode *) inode->private_data)->block[EXT2_SINGLY_INDIRECT_BLOCK_INDEX], 1)) !=
                        1) {
                        ext2_free_blocks(indirect_block);
                        return _ret;
                    }

                    block_no = ((struct raw_inode *) inode->private_data)->block[EXT2_SINGLY_INDIRECT_BLOCK_INDEX];
                } else {
                    size_t real_block_offset = block_index - EXT2_SINGLY_INDIRECT_BLOCK_INDEX - 1;

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

            if (block_index * inode->super_block->block_size <= inode->size) {
#ifdef EXT2_DEBUG
                debug_log("Freeing block: [ %lu ]\n", block_no);
#endif /* EXT2_DEBUG */
                int ret = ext2_free_block(inode->super_block, block_no, false);
                if (ret != 0) {
                    return ret;
                }

                num_blocks_freed++;
            }
            block_index++;
        }

        if (indirect_block != NULL) {
            ext2_free_blocks(indirect_block);
        }

        /* Update bookkepping fields */
        struct ext2_block_group *group =
            ext2_get_block_group(inode->super_block, ext2_get_block_group_from_inode(inode->super_block, inode->index));
        group->blk_desc->num_unallocated_blocks += num_blocks_freed;
        group->blk_desc->num_unallocated_inodes++;
        if (inode->flags & FS_DIR) {
            group->blk_desc->num_directories--;
        }
        int ret = ext2_sync_block_group(inode->super_block, group->index);
        if (ret != 0) {
            return ret;
        }

        struct ext2_sb_data *sb_data = inode->super_block->private_data;
        sb_data->sb->num_unallocated_blocks += num_blocks_freed;
        sb_data->sb->num_unallocated_inodes++;

        fs_mark_super_block_as_dirty(inode->super_block);

        inode->change_time = inode->modify_time = time_read_clock(CLOCK_REALTIME);
        inode->size = new_size;
        fs_mark_inode_as_dirty(inode);
        return 0;
    }

    size_t file_block_no = old_size / inode->super_block->block_size;
    size_t file_block_no_end = (new_size + inode->super_block->block_size - 1) / inode->super_block->block_size;

    uint32_t *indirect_block = NULL;

    while (file_block_no < file_block_no_end) {
        size_t block_no;

        if (file_block_no < EXT2_SINGLY_INDIRECT_BLOCK_INDEX) {
            if (raw_inode->block[file_block_no] == 0) {
                int __ret = ext2_allocate_block_for_inode(inode->super_block, inode, file_block_no);
                if (__ret != 0) {
                    return __ret;
                }
            }

            block_no = raw_inode->block[file_block_no];
        } else {
            /* Handle single indirect block */
            if (indirect_block == NULL) {
                indirect_block = ext2_allocate_blocks(inode->super_block, 1);
                ssize_t _ret;
                if ((_ret = ext2_read_blocks(inode->super_block, indirect_block,
                                             ((struct raw_inode *) inode->private_data)->block[EXT2_SINGLY_INDIRECT_BLOCK_INDEX], 1)) !=
                    1) {
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
        size_t in_block_offset = old_size % inode->super_block->block_size;
        size_t to_write = inode->super_block->block_size - in_block_offset;
        bool read_block = ((size_t) old_size < inode->size) || (in_block_offset != 0);
        if (read_block) {
            int ret = ext2_read_blocks(inode->super_block, buf, block_no, 1);
            if (ret != 1) {
                return (int) ret;
            }
        }

        memset(buf + in_block_offset, 0, to_write);
        int ret = ext2_write_blocks(inode->super_block, buf, block_no, 1);
        if (ret != 1) {
            ext2_free_blocks(buf);
            return -EIO;
        }

        ext2_free_blocks(buf);

        file_block_no++;
        old_size += to_write;
    }

    if (indirect_block) {
        ext2_free_blocks(indirect_block);
    }

    inode->size = new_size;
    inode->change_time = inode->modify_time = time_read_clock(CLOCK_REALTIME);
    fs_mark_inode_as_dirty(inode);
    return 0;
}

void ext2_on_inode_destruction(struct inode *inode) {
    if (inode->private_data) {
        free(inode->private_data);
    }
}

int ext2_read_all(struct inode *inode, void *buffer) {
    if (!inode->private_data) {
        ext2_update_inode(inode, false);
    }

    ssize_t ret = __ext2_read(inode, 0, buffer, inode->size);
    if (ret < 0) {
        return ret;
    }

    return 0;
}

ssize_t ext2_iread(struct inode *inode, void *buffer, size_t len, off_t offset) {
    return __ext2_read(inode, offset, buffer, len);
}

ssize_t ext2_read(struct file *file, off_t offset, void *buffer, size_t len) {
    struct inode *inode = fs_file_inode(file);
    mutex_lock(&inode->lock);

    ssize_t ret = __ext2_read(inode, offset, buffer, len);

    mutex_unlock(&inode->lock);
    return ret;
}

ssize_t ext2_write(struct file *file, off_t offset, const void *buffer, size_t len) {
    struct inode *inode = fs_file_inode(file);
    mutex_lock(&inode->lock);

    ssize_t ret = __ext2_write(inode, offset, buffer, len);

    mutex_unlock(&inode->lock);
    return ret;
}

int ext2_stat(struct inode *inode, struct stat *stat_struct) {
    if (!inode->private_data) {
        ext2_update_inode(inode, false);
    }

    struct raw_inode *raw_inode = inode->private_data;
    stat_struct->st_nlink = raw_inode->link_count;

    return 0;
}

int ext2_link(struct tnode *tparent, const char *name, const struct tnode *target) {
    int error = 0;
    __ext2_create(tparent, name, tparent->inode->mode, &error, target->inode->index, 0);
    if (error < 0) {
        return error;
    }

    struct raw_inode *raw_inode = target->inode->private_data;
    if (!raw_inode) {
        ext2_update_inode(target->inode, true);
        raw_inode = target->inode->private_data;
    }

    raw_inode->link_count++;
    fs_mark_inode_as_dirty(target->inode);
    return 0;
}

struct inode *ext2_symlink(struct tnode *tparent, const char *name, const char *target, int *error) {
    struct inode *inode = __ext2_create(tparent, name, S_IFLNK | 0777, error, 0, 0);
    if (inode == NULL) {
        return NULL;
    }

    size_t target_len = strlen(target);
    if (target_len < 60) {
        struct raw_inode *raw_inode = inode->private_data;
        memcpy(raw_inode->block, target, target_len);

        inode->size = target_len;
        fs_mark_inode_as_dirty(inode);
        return inode;
    }

    // FIXME: allow symlinks longer than 60 characters
    assert(false);
    return inode;
}

struct inode *ext2_mkdir(struct tnode *tparent, const char *name, mode_t mode, int *error) {
    struct inode *inode = __ext2_create(tparent, name, mode, error, 0, 0);
    if (inode == NULL) {
        return NULL;
    }

    struct ext2_block_group *group =
        ext2_get_block_group(inode->super_block, ext2_get_block_group_from_inode(inode->super_block, inode->index));
    group->blk_desc->num_directories++;
    *error = ext2_sync_block_group(inode->super_block, group->index);
    if (*error != 0) {
        free(inode);
        return NULL;
    }

    struct raw_inode *parent_raw_inode = tparent->inode->private_data;
    parent_raw_inode->link_count++;
    fs_mark_inode_as_dirty(tparent->inode);

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
        fs_mark_inode_as_dirty(inode);
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

struct inode *ext2_mknod(struct tnode *tparent, const char *name, mode_t mode, dev_t device, int *error) {
    return __ext2_create(tparent, name, mode, error, 0, device);
}

int __ext2_unlink(struct tnode *tnode, bool drop_reference) {
    struct inode *inode = tnode->inode;

    if (inode->private_data == NULL) {
        ext2_update_inode(inode, true);
    }

    struct raw_inode *raw_inode = inode->private_data;
    assert(raw_inode);

    struct inode *parent = tnode->parent->inode;
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
        assert(!(((uintptr_t) EXT2_NEXT_DIRENT(dirent)) - ((uintptr_t) raw_dirent_table) + (block_no * inode->super_block->block_size) >=
                 parent->size));

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

#ifdef EXT2_DEBUG
    debug_log("Removing inode to from: [ %llu, %s ]\n", inode->index, tnode->parent->name);
#endif /* EXT2_DEBUG */

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

    if (drop_reference) {
        if (--raw_inode->link_count <= 0) {
#ifdef EXT2_DEBUG
            debug_log("Destroying raw ext2 inode: [ %llu ]\n", inode->index);
#endif /* EXT2_DEBUG */

            /* Actually free inode from disk */
            ext2_free_inode(inode->super_block, inode->index, false);

            size_t num_blocks = 0;
            if (!(raw_inode->mode & S_IFLNK) || (inode->size >= 60)) {
                /* Free all of the blocks the inode used */
                uint32_t *indirect_block = NULL;
                for (;;) {
                    size_t block_no;

                    if (num_blocks < EXT2_SINGLY_INDIRECT_BLOCK_INDEX) {
                        block_no = ((struct raw_inode *) inode->private_data)->block[num_blocks];
                    } else {
                        /* Handle single indirect block */
                        if (indirect_block == NULL) {
                            indirect_block = ext2_allocate_blocks(inode->super_block, 1);
                            ssize_t _ret;
#ifdef EXT2_DEBUG
                            debug_log("Indirect block: [ %u ]\n",
                                      ((struct raw_inode *) inode->private_data)->block[EXT2_SINGLY_INDIRECT_BLOCK_INDEX]);
#endif /* EXT2_DEBUG                                                                                                                  */
                            if ((_ret = ext2_read_blocks(
                                     inode->super_block, indirect_block,
                                     ((struct raw_inode *) inode->private_data)->block[EXT2_SINGLY_INDIRECT_BLOCK_INDEX], 1)) != 1) {
                                ext2_free_blocks(indirect_block);
                                return _ret;
                            }

                            block_no = ((struct raw_inode *) inode->private_data)->block[EXT2_SINGLY_INDIRECT_BLOCK_INDEX];
                        } else {
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

#ifdef EXT2_DEBUG
                    debug_log("Freeing block: [ %lu ]\n", block_no);
#endif /* EXT2_DEBUG                                                                */
                    int ret = ext2_free_block(inode->super_block, block_no, false);
                    if (ret != 0) {
                        return ret;
                    }

                    num_blocks++;
                }

                if (indirect_block != NULL) {
                    ext2_free_blocks(indirect_block);
                }
            }

            /* Update bookkepping fields */
            struct ext2_block_group *group =
                ext2_get_block_group(inode->super_block, ext2_get_block_group_from_inode(inode->super_block, inode->index));
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

            fs_mark_super_block_as_dirty(inode->super_block);

            // NOTE: it's fine to call drop_inode_reference with the lock here
            //       because the tnode still has a reference and therefore the
            //       inode will never actually be freed by this call.
            drop_inode_reference(inode);
            return 0;
        }

        fs_mark_inode_as_dirty(inode);
    }

    return 0;
}

int ext2_unlink(struct tnode *tnode) {
    return __ext2_unlink(tnode, true);
}

int ext2_rmdir(struct tnode *tnode) {
    assert(fs_get_dirent_cache_size(tnode->inode->dirent_cache) == 0);

    /* Can't delete reserved inodes */
    if (tnode->inode->index <= ((struct ext2_sb_data *) tnode->inode->super_block->private_data)->sb->first_non_reserved_inode) {
        return -EINVAL;
    }

    struct raw_inode *raw_inode = tnode->inode->private_data;
    struct inode *parent = tnode->parent->inode;
    struct raw_inode *parent_raw_inode = parent->private_data;

    /* Drop .. reference */
    mutex_lock(&parent->lock);
    parent_raw_inode->link_count--;
    assert(parent_raw_inode->link_count > 0);

    fs_mark_inode_as_dirty(parent);
    mutex_unlock(&parent->lock);

    /* Drop . reference */
    raw_inode->link_count--;
    return ext2_unlink(tnode);
}

int ext2_chown(struct inode *inode, uid_t uid, gid_t gid) {
    if (!inode->private_data) {
        ext2_update_inode(inode, true);
    }

    inode->uid = uid;
    inode->gid = gid;
    inode->change_time = inode->modify_time = time_read_clock(CLOCK_REALTIME);
    fs_mark_inode_as_dirty(inode);
    return 0;
}

int ext2_chmod(struct inode *inode, mode_t mode) {
    if (!inode->private_data) {
        ext2_update_inode(inode, true);
    }

    inode->mode = mode;
    inode->change_time = inode->modify_time = time_read_clock(CLOCK_REALTIME);
    fs_mark_inode_as_dirty(inode);
    return 0;
}

int ext2_utimes(struct inode *inode, const struct timespec *times) {
    if (!inode->private_data) {
        ext2_update_inode(inode, true);
    }

    if (times[0].tv_nsec != UTIME_OMIT) {
        inode->access_time = times[0];
    }

    if (times[1].tv_nsec != UTIME_OMIT) {
        inode->modify_time = times[1];
    }

    fs_mark_inode_as_dirty(inode);
    return 0;
}

int ext2_rename(struct tnode *tnode, struct tnode *new_parent, const char *new_name) {
    int error = 0;
    tnode->inode->change_time = time_read_clock(CLOCK_REALTIME);
    __ext2_create(new_parent, new_name, tnode->inode->mode, &error, tnode->inode->index, 0);
    if (error != 0) {
        return error;
    }

    if (tnode->inode->flags & FS_DIR) {
        struct inode *parent = tnode->parent->inode;
        struct raw_inode *parent_raw_inode = parent->private_data;

        /* Drop .. reference */
        mutex_lock(&tnode->parent->inode->lock);
        parent_raw_inode->link_count--;
        assert(parent_raw_inode->link_count > 0);

        parent->change_time = time_read_clock(CLOCK_REALTIME);
        fs_mark_inode_as_dirty(parent);
        mutex_unlock(&parent->lock);
    }

    return __ext2_unlink(tnode, false);
}

int ext2_determine_fsid(struct block_device *block_device, struct block_device_id *result) {
    mutex_lock(&block_device->device->lock);
    struct phys_page *page = block_device->op->read_page(block_device, 0);
    mutex_unlock(&block_device->device->lock);

    if (!page) {
        return -EIO;
    }

    struct ext2_raw_super_block *raw_sb = create_phys_addr_mapping(page->phys_addr) + EXT2_SUPER_BLOCK_OFFSET;
    *result = block_device_id_uuid(raw_sb->fs_uuid);
    drop_phys_page(page);
    return 0;
}

int ext2_mount(struct block_device *device, unsigned long, const void *, struct super_block **super_block_p) {
    if (!device) {
        return -ENODEV;
    }

    struct inode *root = calloc(1, sizeof(struct inode));
    struct super_block *super_block = calloc(1, sizeof(struct super_block));
    struct ext2_sb_data *data = calloc(1, sizeof(struct ext2_sb_data));

    super_block->fsid = device->device->device_number;
    super_block->op = &s_op;
    super_block->root = root;
    init_mutex(&super_block->super_block_lock);
    super_block->block_size = EXT2_SUPER_BLOCK_SIZE; // Set this as defulat for first read
    super_block->device = device->device;
    super_block->private_data = data;
    super_block->flags = 0;

    struct ext2_raw_super_block *raw_super_block = ext2_allocate_blocks(super_block, 1);
    if (__ext2_read_blocks(super_block, raw_super_block, EXT2_SUPER_BLOCK_OFFSET / EXT2_SUPER_BLOCK_SIZE, 1) != 1) {
        debug_log("Ext2 Read Error: [ Super Block ]\n");
        ext2_free_blocks(raw_super_block);
        return -EINVAL;
    }

    data->sb = raw_super_block;
    data->num_block_groups =
        (raw_super_block->num_blocks + raw_super_block->num_blocks_in_block_group - 1) / raw_super_block->num_blocks_in_block_group;
    data->block_groups = calloc(data->num_block_groups, sizeof(struct ext2_block_group));
    super_block->block_size = 1024 << raw_super_block->shifted_blck_size;

    char uuid_string[UUID_STRING_MAX];
    uuid_to_string(raw_super_block->fs_uuid, uuid_string, sizeof(uuid_string));
    debug_log("Ext2 UUID: [ %s ]\n", uuid_string);
    debug_log("Ext2 Num Inodes in Block Group: [ %u ]\n", raw_super_block->num_inodes_in_block_group);
    debug_log("Ext2 Num Blocks: [ %u ]\n", raw_super_block->num_blocks);
    debug_log("Ext2 Num Inodes: [ %u ]\n", raw_super_block->num_inodes);
    debug_log("Ext2 Num Inodes in Group: [ %u ]\n", raw_super_block->num_inodes_in_block_group);
    debug_log("Ext2 Num Blocks in Group: [ %u ]\n", raw_super_block->num_blocks_in_block_group);
    debug_log("Ext2 Block Size: [ %lu ]\n", super_block->block_size);
    debug_log("Ext2 Num Block Groups: [ %lu ]\n", data->num_block_groups);
    debug_log("Ext2 Inode Size: [ %u ]\n", raw_super_block->inode_size);

    blkcnt_t num_blocks =
        (data->num_block_groups * sizeof(struct raw_block_group_descriptor) + super_block->block_size - 1) / super_block->block_size;
    struct raw_block_group_descriptor *raw_block_group_descriptor_table = ext2_allocate_blocks(super_block, num_blocks);
    uint32_t block_group_descriptor_offset = super_block->block_size == 1024 ? 2 : 1;
    if (__ext2_read_blocks(super_block, raw_block_group_descriptor_table, block_group_descriptor_offset, num_blocks) != num_blocks) {
        debug_log("Ext2 Read Error: [ Block Group Descriptor Table ]\n");
        ext2_free_blocks(raw_super_block);
        ext2_free_blocks(raw_block_group_descriptor_table);
        return -EINVAL;
    }

    data->blk_desc_table = raw_block_group_descriptor_table;

    root->fsid = super_block->fsid;
    root->flags = FS_DIR;
    root->i_op = &ext2_dir_i_op;
    root->index = EXT2_ROOT_INODE;
    init_mutex(&root->lock);
    root->mode = S_IFDIR | 0777;
    root->private_data = NULL;
    root->ref_count = 2;
    root->size = 0;
    root->super_block = super_block;
    init_file_state(&root->file_state, true, true);

    ext2_sync_raw_super_block_with_virtual_super_block(super_block);

    *super_block_p = super_block;
    return 0;
}

int ext2_umount(struct super_block *super_block) {
    struct ext2_sb_data *data = super_block->private_data;

    drop_inode_reference(super_block->private_data);

    ext2_free_blocks(data->sb);
    ext2_free_blocks(data->blk_desc_table);

    for (size_t i = 0; i < data->num_block_groups; i++) {
        if (data->block_groups[i].initialized) {
            kill_bitset(&data->block_groups[i].block_bitset);
            kill_bitset(&data->block_groups[i].inode_bitset);
        }
    }
    free(data->block_groups);

    free(data);
    free(super_block);
    return 0;
}

static void init_ext2() {
    register_fs(&fs);
}
INIT_FUNCTION(init_ext2, fs);
