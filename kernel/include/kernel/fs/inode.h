#ifndef _KERNEL_FS_INODE_H
#define _KERNEL_FS_INODE_H 1

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include <kernel/fs/super_block.h>
#include <kernel/fs/file.h>
#include <kernel/util/spinlock.h>

#include <kernel/fs/tnode.h>

struct inode;

struct inode_operations {
    struct tnode *(*lookup)(struct inode *, const char *name);
    struct file *(*open)(struct inode *);
};

struct inode {
    mode_t mode;

#define FS_FILE 1
#define FS_DIR 2
    unsigned int flags;

    struct inode_operations *i_op;
    struct super_block *super_block;

    dev_t device;
    unsigned int size;

    /* Unique inode identifier */
    uint64_t index;

    /* List of tnodes in directory (if inode is a directory) */
    struct tnode_list *tnode_list;

    spinlock_t lock;

    void *private_data;
};

#endif /* _KERNEL_FS_INODE_H */