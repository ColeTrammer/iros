#ifndef _KERNEL_FS_VFS_H
#define _KERNEL_FS_VFS_H 1

#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <kernel/fs/file_system.h>
#include <kernel/fs/inode.h>

struct file_descriptor;
struct iovec;

#define INAME_DONT_FOLLOW_TRAILING_SYMLINK 1
#define INAME_TAKE_OWNERSHIP_OF_PATH       2

void drop_inode_reference_unlocked(struct inode *inode);
void drop_inode_reference(struct inode *inode);
int iname(const char *path, int flags, struct tnode **result);

int fs_read_all_inode_with_buffer(struct inode *inode, void *buffer);
int fs_read_all_inode(struct inode *inode, void **buffer, size_t *buffer_len);
int fs_read_all_path(const char *path, void **buffer, size_t *buffer_len, struct inode **inode);

struct tnode *fs_root(void);
int fs_create(const char *path, mode_t mode);
struct file *fs_open(const char *file_name, int flags, int *error);
int fs_close(struct file *file);
ssize_t fs_read(struct file *file, void *buffer, size_t len);
ssize_t fs_write(struct file *file, const void *buffer, size_t len);
ssize_t fs_pread(struct file *file, void *buffer, size_t len, off_t offset);
ssize_t fs_pwrite(struct file *file, const void *buffer, size_t len, off_t offset);
ssize_t fs_readv(struct file *file, const struct iovec *vec, int item_count);
ssize_t fs_writev(struct file *file, const struct iovec *vec, int item_count);
off_t fs_seek(struct file *file, off_t offset, int whence);
long fs_tell(struct file *file);
int fs_stat(const char *file_name, struct stat *stat_struct);
int fs_lstat(const char *path, struct stat *stat_struct);
int fs_ioctl(struct file *file, unsigned long request, void *argp);
int fs_truncate(struct file *file, off_t length);
int fs_mkdir(const char *path, mode_t mode);
int fs_create_pipe(struct file *pipe_files[2]);
int fs_unlink(const char *path);
int fs_rmdir(const char *path);
int fs_chmod(const char *path, mode_t mode);
int fs_chown(const char *path, uid_t uid, gid_t gid);
int fs_access(const char *path, int mode);
int fs_fcntl(struct file_descriptor *desc, int command, int arg);
int fs_fstat(struct file *file, struct stat *stat_struct);
int fs_fchmod(struct file *file, mode_t mode);
intptr_t fs_mmap(void *addr, size_t length, int prot, int flags, struct file *file, off_t offset);
int fs_rename(const char *old_path, const char *new_path);
ssize_t fs_readlink(const char *path, char *buf, size_t bufsiz);
int fs_symlink(const char *target, const char *linkpath);
int fs_link(const char *oldpath, const char *newpath);
int fs_utimes(const char *path, const struct timeval *times);
int fs_mount(const char *src, const char *path, const char *type);

struct file_descriptor fs_clone(struct file_descriptor desc);
struct file_descriptor fs_dup(struct file_descriptor desc);

int fs_bind_socket_to_inode(struct inode *inode, unsigned long socket_id);

bool fs_is_readable(struct file *file);
bool fs_is_writable(struct file *file);
bool fs_is_exceptional(struct file *file);

void load_fs(struct file_system *fs);

char *get_tnode_path(struct tnode *tnode);

void init_vfs();

static inline int fs_mode_to_flags(mode_t mode) {
    if (S_ISREG(mode)) {
        return FS_FILE;
    } else if (S_ISDIR(mode)) {
        return FS_DIR;
    } else if (S_ISFIFO(mode)) {
        return FS_FIFO;
    } else if (S_ISLNK(mode)) {
        return FS_LINK;
    } else if (S_ISSOCK(mode)) {
        return FS_SOCKET;
    } else {
        return 0;
    }
}

#endif /* _KERNEL_FS_VFS_H */