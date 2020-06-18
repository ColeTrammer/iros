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
#include <kernel/util/spinlock.h>

struct device;
struct vm_object;

struct device_ops {
    struct file *(*open)(struct device *device, int flags, int *error);
    ssize_t (*read)(struct device *device, off_t offset, void *buffer, size_t len);
    ssize_t (*write)(struct device *device, off_t offset, const void *buffer, size_t len);
    int (*close)(struct device *device);
    void (*add)(struct device *device);
    void (*remove)(struct device *device);
    int (*ioctl)(struct device *device, unsigned long request, void *argp);
    void (*on_open)(struct device *device);
    intptr_t (*mmap)(struct device *device, void *addr, size_t len, int prot, int flags, off_t offset);
    int (*read_all)(struct device *device, void *buf);
    blksize_t (*block_size)(struct device *device);
    blkcnt_t (*block_count)(struct device *device);
};

struct device {
    dev_t device_number;
    mode_t type;
    bool cannot_open;
    bool readable;
    bool writeable;
    bool exceptional;
    struct device_ops *ops;
    struct vm_object *vm_object;
    spinlock_t lock;
    void *private;
};

void dev_register(struct device *device);
void dev_unregister(struct device *device);

struct device *dev_get_device(dev_t device_number);

void init_dev();

struct inode *dev_lookup(struct inode *inode, const char *name);
struct file *dev_open(struct inode *inode, int flags, int *error);
int dev_close(struct file *file);
ssize_t dev_read(struct file *file, off_t offset, void *buffer, size_t len);
ssize_t dev_write(struct file *file, off_t offset, const void *buffer, size_t len);
int dev_ioctl(struct inode *inode, unsigned long request, void *argp);
intptr_t dev_mmap(void *addr, size_t len, int prot, int flags, struct inode *inode, off_t offset);
int dev_read_all(struct inode *inode, void *buf);
blksize_t dev_block_size(struct device *device);
blkcnt_t dev_block_count(struct device *device);
struct inode *dev_mount(struct file_system *fs, char *device_path);

#endif /* _KERNEL_FS_DEV_H */
