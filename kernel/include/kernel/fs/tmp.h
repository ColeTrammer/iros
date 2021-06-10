#ifndef _KERNEL_FS_TMP_H
#define _KERNEL_FS_TMP_H 1

#include <stdint.h>

#include <kernel/fs/file_system.h>
#include <kernel/fs/tnode.h>

#define TMP_MAX_FILE_NAME_LENGTH 255

ino_t tmp_get_next_index(void);
struct inode *tmp_mknod(struct tnode *tparent, const char *name, mode_t mode, dev_t device, int *error);
struct inode *tmp_lookup(struct inode *inode, const char *name);
struct file *tmp_open(struct inode *inode, int flags, int *error);
ssize_t tmp_read(struct file *file, off_t offset, void *buffer, size_t len);
ssize_t tmp_write(struct file *file, off_t offset, const void *buffer, size_t len);
struct inode *tmp_mkdir(struct tnode *tparent, const char *name, mode_t mode, int *error);
struct inode *tmp_symlink(struct tnode *tnode, const char *name, const char *target, int *error);
int tmp_unlink(struct tnode *tnode);
int tmp_rmdir(struct tnode *tnode);
intptr_t tmp_mmap(void *addr, size_t len, int prot, int flags, struct inode *inode, off_t offset);
int tmp_mount(struct block_device *device, unsigned long flags, const void *data, struct super_block **super_block);
int tmp_chmod(struct inode *inode, mode_t mode);
int tmp_chown(struct inode *inode, uid_t uid, gid_t gid);
int tmp_utimes(struct inode *inode, const struct timespec *times);
int tmp_rename(struct tnode *tnode, struct tnode *new_parent, const char *new_name);
int tmp_read_all(struct inode *inode, void *buffer);
int tmp_truncate(struct inode *inode, off_t len);
void tmp_on_inode_destruction(struct inode *inode);

extern struct inode_operations tmp_i_op;

#endif /* _KERNEL_FS_TMP_H */
