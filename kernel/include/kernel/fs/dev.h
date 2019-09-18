#ifndef _KERNEL_FS_DEV_H
#define _KERNEL_FS_DEV_H 1

#include <sys/types.h>
#include <stddef.h>

#include <kernel/fs/file_system.h>
#include <kernel/fs/file.h>
#include <kernel/fs/inode.h>
#include <kernel/fs/tnode.h>

struct device;

struct device_ops {
    int (*open)(struct device *device, struct file *file);
    ssize_t (*read)(struct device *device, struct file *file, void *buffer, size_t len);
    ssize_t (*write)(struct device *device, struct file *file, const void *buffer, size_t len); 
    int (*close)(struct device *device);
    void (*add)(struct device *device);
    void (*remove)(struct device *device);
};

struct device {
    dev_t device_number;
    char name[16];
    struct device_ops *ops;
    void *private;
};

void dev_add(struct device *device, const char *path);
void dev_remove(const char *path);

void init_dev();

struct tnode *dev_lookup(struct inode *inode, const char *name);
struct file *dev_open(struct inode *inode, int *error);
int dev_close(struct file *file);
ssize_t dev_read(struct file *file, void *buffer, size_t len);
ssize_t dev_write(struct file *file, const void *buffer, size_t len);
struct tnode *dev_mount(struct file_system *fs);

#endif /* _KERNEL_FS_DEV_H */