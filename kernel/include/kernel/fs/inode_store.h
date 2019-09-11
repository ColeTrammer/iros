#ifndef _KERNEL_FS_INODE_STORE_H
#define _KERNEL_FS_INODE_STORE_H 1

#include <kernel/fs/inode.h>

#define INODE_STORE_HASH_SIZE 0x1000

struct hash_entry {
	inode_id_t id;
	struct hash_entry *next;
	struct inode *inode;
};

struct inode *fs_inode_get(inode_id_t id);
void fs_inode_put(struct inode *inode);
void fs_inode_set(struct inode *inode);
void fs_inode_del(inode_id_t id);
void fs_inode_free_hash_table();

#endif /* _KERNEL_FS_INODE_STORE_H */