#ifndef _KERNEL_FS_FILE_H
#define _KERNEL_FS_FILE_H 1

#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>

#include <kernel/fs/inode.h>

struct file;

struct file_operations {
    void (*close)(struct file *);
    void (*read)(struct file *, void *, size_t);
    void (*write)(struct file *, const void *, size_t);
};

struct file {
    inode_id_t inode_idenifier;
    
    off_t length;
    uintptr_t start;
    fpos_t position;

    struct file_operations *f_op;

    dev_t device;
};

#endif /* _KERNEL_FS_FILE_H */