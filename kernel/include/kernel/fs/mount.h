#ifndef _KERNEL_FS_MOUNT_H
#define _KERNEL_FS_MOUNT_H 1

#include <kernel/util/list.h>

struct file_system;
struct super_block;

struct mount {
    char *name;
    struct file_system *fs;
    struct super_block *super_block;
    struct list_node list;
};

#endif /* _KERNEL_FS_MOUNT_H */
