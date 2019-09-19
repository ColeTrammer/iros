#ifndef _KERNEL_FS_EXT2_H
#define _KERNEL_FS_EXT2_H 1

#include <sys/types.h>
#include <stddef.h>

#include <kernel/fs/file_system.h>
#include <kernel/fs/tnode.h>

struct tnode *ext2_lookup(struct inode *inode, const char *name);
struct file *ext2_open(struct inode *inode, int *error);
int ext2_close(struct file *file);
ssize_t ext2_read(struct file *file, void *buffer, size_t len);
ssize_t ext2_write(struct file *file, const void *buffer, size_t len);
struct tnode *ext2_mount(struct file_system *fs, char *device_path);

void init_ext2();

#endif /* _KERNEL_FS_EXT2_H */