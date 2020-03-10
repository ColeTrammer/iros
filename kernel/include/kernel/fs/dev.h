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

struct device;

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
};

struct device {
    dev_t device_number;
    mode_t type;
    char name[16];
    bool cannot_open;
    struct device_ops *ops;
    struct inode *inode;
    void *private;
};

void dev_add(struct device *device, const char *path);
void dev_remove(const char *path);

void init_dev();

struct inode *dev_lookup(struct inode *inode, const char *name);
struct file *dev_open(struct inode *inode, int flags, int *error);
int dev_close(struct file *file);
ssize_t dev_read(struct file *file, off_t offset, void *buffer, size_t len);
ssize_t dev_write(struct file *file, off_t offset, const void *buffer, size_t len);
int dev_stat(struct inode *inode, struct stat *stat_struct);
int dev_ioctl(struct inode *inode, unsigned long request, void *argp);
intptr_t dev_mmap(void *addr, size_t len, int prot, int flags, struct inode *inode, off_t offset);
int dev_read_all(struct inode *inode, void *buf);
struct tnode *dev_mount(struct file_system *fs, char *device_path);

dev_t dev_get_device_number(struct file *file);

#endif /* _KERNEL_FS_DEV_H */