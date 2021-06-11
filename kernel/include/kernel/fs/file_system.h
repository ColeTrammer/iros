#ifndef _KERNEL_FS_FILE_SYSTEM_H
#define _KERNEL_FS_FILE_SYSTEM_H 1

#include <kernel/fs/super_block.h>
#include <kernel/fs/tnode.h>
#include <kernel/hal/block.h>
#include <kernel/util/list.h>

struct block_device;

struct file_system {
    char name[8];
    unsigned int flags;
    int (*mount)(struct block_device *block_device, unsigned long flags, const void *data, struct super_block **super_block);
    int (*umount)(struct super_block *super_block);
    int (*determine_fsid)(struct block_device *block_device, struct block_device_id *result);
    struct list_node list;
    struct block_device_id *id_table;
    size_t id_count;
};

int fs_show_file_system(struct file_system *fs, char *buffer, size_t buffer_length);

static inline bool fs_id_matches_file_system(struct block_device_id id, struct file_system *fs) {
    for (size_t i = 0; i < fs->id_count; i++) {
        if (block_device_id_equals(fs->id_table[i], id)) {
            return true;
        }
    }
    return false;
}

#endif /* _KERNEL_FS_FILE_SYSTEM_H */
