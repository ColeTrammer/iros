#ifndef _KERNEL_FS_DEV_H
#define _KERNEL_FS_DEV_H 1

#include <sys/types.h>
#include <sys/stat.h>
#include <stddef.h>

#include <kernel/fs/file_system.h>
#include <kernel/fs/inode.h>
#include <kernel/fs/tnode.h>
#include <kernel/fs/file.h>

struct device;

struct device_ops {
    int (*open)(struct device *device, struct file *file);
    ssize_t (*read)(struct device *device, struct file *file, void *buffer, size_t len);
    ssize_t (*write)(struct device *device, struct file *file, const void *buffer, size_t len); 
    int (*close)(struct device *device);
    void (*add)(struct device *device);
    void (*remove)(struct device *device);
    int (*ioctl)(struct device *device, unsigned long request, void *argp);
};

struct device {
    dev_t device_number;
    mode_t type;
    char name[16];
    struct device_ops *ops;
    void *private;
};

void dev_add(struct device *device, const char *path);
void dev_remove(const char *path);

void init_dev();

struct tnode *dev_lookup(struct inode *inode, const char *name);
struct file *dev_open(struct inode *inode, int flags, int *error);
int dev_close(struct file *file);
ssize_t dev_read(struct file *file, void *buffer, size_t len);
ssize_t dev_write(struct file *file, const void *buffer, size_t len);
int dev_stat(struct inode *inode, struct stat *stat_struct);
int dev_ioctl(struct inode *inode, unsigned long request, void *argp);
struct tnode *dev_mount(struct file_system *fs, char *device_path);

dev_t dev_get_device_number(struct file *file);

#endif /* _KERNEL_FS_DEV_H */