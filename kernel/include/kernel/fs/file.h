#ifndef _KERNEL_FS_FILE_H
#define _KERNEL_FS_FILE_H 1

#include <sys/types.h>
#include <stdint.h>

#include <kernel/fs/inode.h>

struct file;

struct file_operations {
    int (*close)(struct file *file);
    ssize_t (*read)(struct file *file, void *buffer, size_t len);
    ssize_t (*write)(struct file *file, const void *buffer, size_t len);
};

struct file {
    ino_t inode_idenifier;
    
    off_t length;
    uintptr_t start;
    uintptr_t position;

    struct file_operations *f_op;
    unsigned int flags;

    dev_t device;
};

#endif /* _KERNEL_FS_FILE_H */