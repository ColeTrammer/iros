#ifndef _KERNEL_FS_MOUNT_H
#define _KERNEL_FS_MOUNT_H 1

#include <sys/types.h>

#include <kernel/fs/super_block.h>

struct mount {
    dev_t device;
    const char *name;
    struct super_block *super_block;
    struct mount *next;
};

#endif /* _KERNEL_FS_MOUNT_H */