#ifndef _KERNEL_FS_DEV_H
#define _KERNEL_FS_DEV_H 1

#include <sys/types.h>
#include <stddef.h>

#include <kernel/fs/file_system.h>
#include <kernel/fs/file.h>
#include <kernel/fs/inode.h>
#include <kernel/fs/tnode.h>

void init_dev();

struct tnode *dev_lookup(struct inode *inode, const char *name);
struct file *dev_open(struct inode *inode);
void dev_close(struct file *file);
void dev_read(struct file *file, void *buffer, size_t len);
void dev_write(struct file *file, const void *buffer, size_t len);
struct tnode *dev_mount(struct file_system *fs);

#endif /* _KERNEL_FS_DEV_H */