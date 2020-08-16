#ifndef _KERNEL_FS_SUPER_BLOCK_H
#define _KERNEL_FS_SUPER_BLOCK_H 1

#include <stdint.h>
#include <sys/types.h>

#include <kernel/util/list.h>
#include <kernel/util/mutex.h>

struct device;
struct super_block;
struct tnode;

struct super_block_operations {
    int (*rename)(struct tnode *tnode, struct tnode *new_parent, const char *new_name);
    int (*sync)(struct super_block *sb);
};

struct super_block {
    dev_t fsid;
    struct inode *root;
    struct super_block_operations *op;
    blksize_t block_size;

    fsblkcnt_t num_blocks;
    fsblkcnt_t free_blocks;
    fsblkcnt_t available_blocks;

    fsfilcnt_t num_inodes;
    fsfilcnt_t free_inodes;
    fsfilcnt_t available_inodes;

    struct list_node dirty_super_blocks;

    int flags;
    bool dirty : 1;

    struct device *device;
    mutex_t super_block_lock;

    void *private_data;
};

#endif /* _KERNEL_FS_SUPER_BLOCK_H */
