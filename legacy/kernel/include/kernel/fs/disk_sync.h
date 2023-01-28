#ifndef _KERNEL_FS_DISK_SYNC_H
#define _KERNEL_FS_DISK_SYNC_H 1

struct inode;
struct super_block;

void fs_mark_inode_as_dirty(struct inode *inode);
void fs_mark_super_block_as_dirty(struct super_block *sb);

void init_disk_sync_task(void);

#endif /* _KERNEL_FS_DISK_SYNC_H */
