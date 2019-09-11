#ifndef _KERNEL_FS_FILE_SYSTEM_H
#define _KERNEL_FS_FILE_SYSTEM_H 1

#include <stddef.h>

#include <kernel/fs/tnode.h>
#include <kernel/fs/super_block.h>

struct file_system {
    char name[8];
    unsigned int flags;
    struct tnode *(*mount) (struct file_system *);

    struct super_block *super_block;
    struct file_system *next;
};

#endif /* _KERNEL_FS_FILE_SYSTEM_H */