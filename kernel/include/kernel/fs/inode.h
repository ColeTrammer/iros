#ifndef _KERNEL_FS_INODE_H
#define _KERNEL_FS_INODE_H 1

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <kernel/fs/super_block.h>
#include <kernel/util/spinlock.h>
#include <kernel/fs/mount.h>
#include <kernel/fs/tnode.h>

struct inode;

/* Has to be included here so that file.h sees struct inode & ino_t */
#include <kernel/fs/file.h>

struct inode_operations {
    struct inode *(*create)(struct tnode *tnode, const char *name, mode_t mode, int *error);
    struct tnode *(*lookup)(struct inode *inode, const char *name);
    struct file *(*open)(struct inode *inode, int flags, int *error);
    int (*stat)(struct inode *inode, struct stat *stat_struct);
    int (*ioctl)(struct inode *inode, unsigned long request, void *argp);
    struct inode *(*mkdir)(struct tnode *tnode, const char *name, mode_t mode, int *error);
    int (*unlink)(struct tnode *tnode);
    int (*rmdir)(struct tnode *tnode);
    int (*chmod)(struct inode *inode, mode_t mode);
    intptr_t (*mmap)(void *addr, size_t len, int prot, int flags, struct inode *inode, off_t offset);
    void (*on_inode_destruction)(struct inode *inode);
};

struct inode {
    mode_t mode;

#define FS_FILE   1U
#define FS_DIR    2U
#define FS_FIFO   4U
#define FS_SOCKET 8U
    unsigned int flags;

    struct inode_operations *i_op;
    struct super_block *super_block;

    // Id of the socket this socket is bound to (will be 0 if unboud)
    unsigned long socket_id;

    /* Device id of filesystem */
    dev_t device;

    /* File system size */
    size_t size;

    /* Unique inode identifier (for the filesystem) */
    ino_t index;

    /* List of tnodes in directory (if inode is a directory) */
    struct tnode_list *tnode_list;

    /* Listens file systems mounted directly below this inode */
    struct mount *mounts;

    /* Parent of inode */
    struct tnode *parent;

    // Delete inode when count is 0 (only applies to pipes right now) 
    // Should be atomic 
    int ref_count;

    spinlock_t lock;

    void *private_data;
};

#endif /* _KERNEL_FS_INODE_H */