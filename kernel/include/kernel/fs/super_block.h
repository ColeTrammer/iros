#ifndef _KERNEL_FS_SUPER_BLOCK_H
#define _KERNEL_FS_SUPER_BLOCK_H 1

#include <stdint.h>
#include <sys/types.h>

#include <kernel/fs/file.h>
#include <kernel/util/spinlock.h>

struct tnode;

struct super_block_operations {
    int (*rename)(struct tnode *tnode, struct tnode *new_parent, const char *new_name);
};

struct super_block {
    dev_t device;
    struct tnode *root;
    struct super_block_operations *op;
    blksize_t block_size;

    fsblkcnt_t num_blocks;
    fsblkcnt_t free_blocks;
    fsblkcnt_t available_blocks;

    fsfilcnt_t num_inodes;
    fsfilcnt_t free_inodes;
    fsfilcnt_t available_inodes;

    int flags;

    struct file *dev_file;
    spinlock_t super_block_lock;

    void *private_data;
};

#endif /* _KERNEL_FS_SUPER_BLOCK_H */