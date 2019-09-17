#ifndef _KERNEL_FS_INODE_H
#define _KERNEL_FS_INODE_H 1

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include <kernel/fs/super_block.h>
#include <kernel/util/spinlock.h>
#include <kernel/fs/mount.h>
#include <kernel/fs/tnode.h>

struct inode;

typedef unsigned long inode_id_t;

/* Has to be included here so that file.h sees struct inode & inode_id_t */
#include <kernel/fs/file.h>

struct inode_operations {
    struct tnode *(*lookup)(struct inode *inode, const char *name);
    struct file *(*open)(struct inode *inode, int *error);
};

struct inode {
    mode_t mode;

#define FS_FILE 1U
#define FS_DIR 2U
    unsigned int flags;

    struct inode_operations *i_op;
    struct super_block *super_block;

    dev_t device;
    unsigned int size;

    /* Unique inode identifier */
    inode_id_t index;

    /* List of tnodes in directory (if inode is a directory) */
    struct tnode_list *tnode_list;

    /* Listens file systems mounted directly below this inode */
    struct mount *mounts;

    /* Parent of inode */
    struct tnode *parent;

    spinlock_t lock;

    void *private_data;
};

#endif /* _KERNEL_FS_INODE_H */