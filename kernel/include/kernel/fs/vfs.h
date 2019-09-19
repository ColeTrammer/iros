#ifndef _KERNEL_FS_VFS_H
#define _KERNEL_FS_VFS_H 1

#include <sys/types.h>

#include <kernel/fs/file_system.h>
#include <kernel/fs/inode.h>

void init_vfs();

struct tnode *iname(const char *path);

struct file *fs_open(const char *file_name, int *error);
int fs_close(struct file *file);
ssize_t fs_read(struct file *file, void *buffer, size_t len);
ssize_t fs_write(struct file *file, const void *buffer, size_t len);
int fs_seek(struct file *file, off_t offset, int whence);
long fs_tell(struct file *file);
int fs_mount(const char *src, const char *path, const char *type);
struct file *fs_clone(struct file *file);

void load_fs(struct file_system *fs);

char *get_full_path(char *cwd, const char *relative_path);
char *get_tnode_path(struct tnode *tnode);

#endif /* _KERNEL_FS_VFS_H */