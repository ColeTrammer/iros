#ifndef _KERNEL_FS_DEV_H
#define _KERNEL_FS_DEV_H 1

#include <stdbool.h>
#include <stddef.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <kernel/fs/file.h>
#include <kernel/fs/file_system.h>
#include <kernel/fs/inode.h>
#include <kernel/fs/tnode.h>
#include <kernel/util/hash_map.h>
#include <kernel/util/list.h>
#include <kernel/util/mutex.h>

struct fs_device;
struct vm_object;

struct fs_device_ops {
    struct file *(*open)(struct fs_device *device, int flags, int *error);
    ssize_t (*read)(struct fs_device *device, off_t offset, void *buffer, size_t len, bool non_blocking);
    ssize_t (*write)(struct fs_device *device, off_t offset, const void *buffer, size_t len, bool non_blocking);
    int (*close)(struct fs_device *device);
    void (*add)(struct fs_device *device);
    void (*remove)(struct fs_device *device);
    int (*ioctl)(struct fs_device *device, unsigned long request, void *argp);
    void (*on_open)(struct fs_device *device);
    intptr_t (*mmap)(struct fs_device *device, void *addr, size_t len, int prot, int flags, off_t offset);
    int (*read_all)(struct fs_device *device, void *buf);
    blksize_t (*block_size)(struct fs_device *device);
    blkcnt_t (*block_count)(struct fs_device *device);
};

struct fs_device {
    char name[16];
    dev_t device_number;
    mode_t mode;
    bool cannot_open : 1;
    struct file_state file_state;
    int ref_count;
    struct fs_device_ops *ops;
    struct vm_object *vm_object;
    mutex_t lock;
    struct hash_entry hash;
    void *private;
};

struct hash_map *dev_device_hash_map(void);
struct inode *dev_register(struct fs_device *device);
void dev_unregister(struct fs_device *device);
struct fs_device *dev_bump_device(struct fs_device *device);
void dev_drop_device(struct fs_device *device);
struct fs_device *dev_get_device(dev_t device_number);

struct inode *dev_lookup(struct inode *inode, const char *name);
struct file *dev_open(struct inode *inode, int flags, int *error);
int dev_close(struct file *file);
ssize_t dev_read(struct file *file, off_t offset, void *buffer, size_t len);
ssize_t dev_write(struct file *file, off_t offset, const void *buffer, size_t len);
int dev_ioctl(struct file *file, unsigned long request, void *argp);
intptr_t dev_mmap(void *addr, size_t len, int prot, int flags, struct file *file, off_t offset);
blksize_t dev_block_size(struct fs_device *device);
blkcnt_t dev_block_count(struct fs_device *device);
struct super_block *dev_mount(struct file_system *fs, struct fs_device *device);

#define dev_poll_wait(dev, flags, timeout) fs_poll_wait(&(dev)->file_state, &(dev)->lock, flags, timeout)

#endif /* _KERNEL_FS_DEV_H */
