#ifndef _KERNEL_FS_VFS_H
#define _KERNEL_FS_VFS_H 1

#include <kernel/fs/file_system.h>

void init_vfs();

struct file *fs_open(const char *file_name);
void fs_close(struct file *file);
void fs_read(struct file *file, void *buffer, size_t len);
void fs_write(struct file *file, const void *buffer, size_t len);
int fs_seek(struct file *file, long offset, int whence);
long fs_tell(struct file *file);

void load_fs(struct file_system *fs);

#endif /* _KERNEL_FS_VFS_H */