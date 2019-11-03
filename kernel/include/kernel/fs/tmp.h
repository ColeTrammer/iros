#ifndef _KERNEL_FS_TMP_H
#define _KERNEL_FS_TMP_H 1

#include <stdint.h>

#include <kernel/fs/file_system.h>
#include <kernel/fs/tnode.h>

#define TMP_MAX_FILE_NAME_LENGTH 255

void init_tmpfs();

struct tnode *tmp_lookup(struct inode *inode, const char *name);
struct file *tmp_open(struct inode *inode, int flags, int *error);
ssize_t tmp_read(struct file *file, void *buffer, size_t len);
ssize_t tmp_write(struct file *file, void *buffer, size_t len);
int tmp_stat(struct inode *inode, struct stat *stat_struct);
struct tnode *tmp_mount(struct file_system *fs, char *device_path);

#endif /* _KERNEL_FS_TMP_H */