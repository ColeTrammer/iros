#ifndef _KERNEL_FS_FILE_SYSTEM_H
#define _KERNEL_FS_FILE_SYSTEM_H 1

#include <kernel/fs/super_block.h>
#include <kernel/fs/tnode.h>
#include <kernel/hal/block.h>
#include <kernel/util/list.h>

struct block_device;
struct fs_device;

struct file_system {
    char name[8];
    unsigned int flags;
    struct super_block *(*mount)(struct file_system *self, struct fs_device *block_device);
    int (*determine_fsid)(struct file_system *self, struct block_device *block_device, struct block_device_id *result);
    struct list_node list;
    struct block_device_id *id_table;
    size_t id_count;
};

#endif /* _KERNEL_FS_FILE_SYSTEM_H */
