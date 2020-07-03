#ifndef _KERNEL_FS_FILE_H
#define _KERNEL_FS_FILE_H 1

#include <stdint.h>
#include <sys/types.h>

#include <kernel/fs/inode.h>
#include <kernel/util/mutex.h>

struct file;
struct inode;
struct tnode;

struct file_operations {
    int (*close)(struct file *file);
    ssize_t (*read)(struct file *file, off_t offset, void *buffer, size_t len);
    ssize_t (*write)(struct file *file, off_t offset, const void *buffer, size_t len);
};

struct file {
    off_t position;

    struct file_operations *f_op;
    unsigned int flags;

    int open_flags;

#define FS_FILE_CAN_READ  1
#define FS_FILE_CAN_WRITE 2
#define FS_FILE_CANT_SEEK 4
    int abilities;

    int ref_count;
    mutex_t lock;

    struct tnode *tnode;
    struct inode *inode;

    void *private_data;
};

#endif /* _KERNEL_FS_FILE_H */
