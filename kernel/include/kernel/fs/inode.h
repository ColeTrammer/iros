#ifndef _KERNEL_FS_INODE_H
#define _KERNEL_FS_INODE_H 1

#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <kernel/fs/mount.h>
#include <kernel/fs/super_block.h>
#include <kernel/fs/tnode.h>
#include <kernel/util/spinlock.h>

struct device;
struct hash_map;
struct inode;
struct timeval;

/* Has to be included here so that file.h sees struct inode & ino_t */
#include <kernel/fs/file.h>

struct inode_operations {
    struct inode *(*create)(struct tnode *tnode, const char *name, mode_t mode, int *error);
    struct inode *(*lookup)(struct inode *inode, const char *name);
    struct file *(*open)(struct inode *inode, int flags, int *error);
    int (*stat)(struct inode *inode, struct stat *stat_struct);
    int (*ioctl)(struct inode *inode, unsigned long request, void *argp);
    struct inode *(*mkdir)(struct tnode *tnode, const char *name, mode_t mode, int *error);
    int (*unlink)(struct tnode *tnode);
    int (*rmdir)(struct tnode *tnode);
    int (*chmod)(struct inode *inode, mode_t mode);
    int (*chown)(struct inode *inode, uid_t uid, gid_t gid);
    intptr_t (*mmap)(void *addr, size_t len, int prot, int flags, struct inode *inode, off_t offset);
    struct inode *(*symlink)(struct tnode *tnode, const char *name, const char *target, int *error);
    int (*link)(struct tnode *tnode, const char *name, const struct tnode *target);
    int (*read_all)(struct inode *inode, void *buffer);
    int (*utimes)(struct inode *inode, const struct timespec *times);
    void (*on_inode_destruction)(struct inode *inode);
    ssize_t (*read)(struct inode *inode, void *buffer, size_t size, off_t offset);
};

struct inode {
    mode_t mode;

#define FS_FILE   1U
#define FS_DIR    2U
#define FS_FIFO   4U
#define FS_SOCKET 8U
#define FS_LINK   16U
#define FS_DEVICE 32U
    unsigned int flags;

    struct inode_operations *i_op;
    struct super_block *super_block;

    // Id of the socket this socket is bound to (will be 0 if unboud)
    unsigned long socket_id;

    uid_t uid;
    gid_t gid;

    struct timespec access_time;
    struct timespec modify_time;
    struct timespec change_time;

    /* Device id of filesystem */
    dev_t fsid;

    /* File system size */
    size_t size;

    /* Unique inode identifier (for the filesystem) */
    ino_t index;

    /* Listens file systems mounted directly below this inode */
    struct mount *mounts;

    // flags for things like pselect and blocking
    bool readable : 1;
    bool writeable : 1;
    bool excetional_activity : 1;

    // Delete inode when count is 0 (only applies to pipes right now)
    // Should be atomic
    int ref_count;

    // Dirent cache for path name resolution
    struct hash_map *dirent_cache;

    // Underlying vm_object (used for mmap)
    // Should be lazily initialized
    struct vm_object *vm_object;

    spinlock_t lock;

    void *private_data;
};

#endif /* _KERNEL_FS_INODE_H */
