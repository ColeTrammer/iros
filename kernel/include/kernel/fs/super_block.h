#ifndef _KERNEL_FS_SUPER_BLOCK_H
#define _KERNEL_FS_SUPER_BLOCK_H 1

#include <stdint.h>

#include <kernel/fs/inode.h>

struct super_block_operations {

};

struct super_block {
    dev_t device;
    struct inode *root;
    struct super_block_operations *op;
};

#endif /* _KERNEL_FS_SUPER_BLOCK_H */