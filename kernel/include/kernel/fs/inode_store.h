#ifndef _KERNEL_FS_INODE_STORE_H
#define _KERNEL_FS_INODE_STORE_H 1

#include <kernel/fs/inode.h>

void init_fs_inode_store();
struct inode *fs_inode_get(dev_t dev, ino_t id);
void fs_inode_put(struct inode *inode);
void fs_inode_set(struct inode *inode);
void fs_inode_del(dev_t dev, ino_t id);
void fs_inode_free_hash_table();

ino_t fs_get_next_inode_id();

#endif /* _KERNEL_FS_INODE_STORE_H */