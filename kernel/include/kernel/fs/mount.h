#ifndef _KERNEL_FS_MOUNT_H
#define _KERNEL_FS_MOUNT_H 1

#include <sys/types.h>

#include <kernel/fs/file_system.h>
#include <kernel/fs/super_block.h>

struct fs_device;

struct mount {
    struct fs_device *device;
    const char *name;
    struct file_system *fs;
    struct super_block *super_block;
    struct mount *next;
};

#endif /* _KERNEL_FS_MOUNT_H */
