#ifndef _KERNEL_FS_PROCFS_H
#define _KERNEL_FS_PROCFS_H 1

#include <stdint.h>

#include <kernel/fs/file_system.h>
#include <kernel/fs/tnode.h>

struct tnode *procfs_lookup(struct inode *inode, const char *name);
struct file *procfs_open(struct inode *inode, int flags, int *error);
ssize_t procfs_read(struct file *file, off_t offset, void *buffer, size_t len);
struct tnode *procfs_mount(struct file_system *fs, char *device_path);

void init_procfs();

#endif /* _KERNEL_FS_PROCFS_H */