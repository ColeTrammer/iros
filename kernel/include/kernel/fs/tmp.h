#ifndef _KERNEL_FS_TMP_H
#define _KERNEL_FS_TMP_H 1

#include <stdint.h>

#include <kernel/fs/file_system.h>
#include <kernel/fs/tnode.h>

#define TMP_MAX_FILE_NAME_LENGTH 255

struct tmp_data {
    void *contents;
    size_t max;
    pid_t owner;
};

struct inode *tmp_create(struct tnode *tparent, const char *name, mode_t mode, int *error);
struct tnode *tmp_lookup(struct inode *inode, const char *name);
struct file *tmp_open(struct inode *inode, int flags, int *error);
ssize_t tmp_read(struct file *file, void *buffer, size_t len);
ssize_t tmp_write(struct file *file, const void *buffer, size_t len);
int tmp_stat(struct inode *inode, struct stat *stat_struct);
struct inode *tmp_mkdir(struct tnode *tparent, const char *name, mode_t mode, int *error);
int tmp_unlink(struct tnode *tnode);
int tmp_rmdir(struct tnode *tnode);
struct tnode *tmp_mount(struct file_system *fs, char *device_path);
int tmp_chmod(struct inode *inode, mode_t mode);
int tmp_rename(struct tnode *tnode, struct tnode *new_parent, const char *new_name);
void tmp_on_inode_destruction(struct inode *inode);

void init_tmpfs();

#endif /* _KERNEL_FS_TMP_H */