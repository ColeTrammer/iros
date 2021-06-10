#ifndef _KERNEL_FS_VFS_H
#define _KERNEL_FS_VFS_H 1

#include <poll.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <kernel/fs/file_system.h>
#include <kernel/fs/inode.h>
#include <kernel/proc/wait_queue.h>
#include <kernel/time/timer.h>

struct file_descriptor;
struct file_operations;
struct inode_operations;
struct iovec;
struct statvfs;
struct super_block;

#define PATH_MAX 4096
#define NAME_MAX 255

#define INAME_DONT_FOLLOW_TRAILING_SYMLINK            1
#define INAME_TAKE_OWNERSHIP_OF_PATH                  2
#define INAME_CHECK_PERMISSIONS_WITH_REAL_UID_AND_GID 4

enum fs_root_desc_type {
    FS_ROOT_TYPE_FS_NAME,
    FS_ROOT_TYPE_PARTITION_ID,
    FS_ROOT_TYPE_FS_ID,
};

struct fs_root_desc {
    enum fs_root_desc_type type;
    union {
        const char *fs_name;
        struct block_device_id id;
    };
};

struct inode *bump_inode_reference(struct inode *inode);
void drop_inode_reference(struct inode *inode);
int inode_poll(struct file *file, struct wait_queue_entry *entry, int mask);
void inode_poll_finish(struct file *file, struct wait_queue_entry *entry);

int iname(const char *path, int flags, struct tnode **result);
int iname_with_base(struct tnode *base, const char *_path, int flags, struct tnode **result);

int fs_read_all_inode_with_buffer(struct inode *inode, void *buffer);
int fs_read_all_inode(struct inode *inode, void **buffer, size_t *buffer_len);
int fs_read_all_path(const char *path, void **buffer, size_t *buffer_len, struct tnode **tnode);

struct file *fs_create_file(struct inode *inode, int type, int abilities, int flags, struct file_operations *operations, void *private);
struct inode *fs_create_inode(struct super_block *sb, ino_t id, uid_t uid, gid_t gid, mode_t mode, size_t size,
                              struct inode_operations *ops, void *private);
struct inode *fs_create_inode_without_sb(dev_t fsid, ino_t id, uid_t uid, gid_t gid, mode_t mode, size_t size, struct inode_operations *ops,
                                         void *private);

struct tnode *fs_root(void);
struct file *fs_openat(struct tnode *base, const char *file_name, int flags, mode_t mode, int *error);
int fs_close(struct file *file);
ssize_t fs_read(struct file *file, void *buffer, size_t len);
ssize_t fs_write(struct file *file, const void *buffer, size_t len);
ssize_t fs_pread(struct file *file, void *buffer, size_t len, off_t offset);
ssize_t fs_pwrite(struct file *file, const void *buffer, size_t len, off_t offset);
ssize_t fs_readv(struct file *file, const struct iovec *vec, int item_count);
ssize_t fs_writev(struct file *file, const struct iovec *vec, int item_count);
off_t fs_seek(struct file *file, off_t offset, int whence);
long fs_tell(struct file *file);
int fs_fstatat(struct tnode *base, const char *file_name, struct stat *stat_struct, int flags);
int fs_ioctl(struct file_descriptor *desc, unsigned long request, void *argp);
int fs_truncate(const char *path, off_t length);
int fs_ftruncate(struct file *file, off_t length);
struct tnode *fs_mkdir(const char *path, mode_t mode, int *error);
struct tnode *fs_mknod(const char *path, mode_t mode, dev_t dev, int *error);
int fs_create_pipe(struct file *pipe_files[2]);
int fs_unlink(const char *path, bool ignore_permission_checks);
int fs_rmdir(const char *path);
int fs_fchmodat(struct tnode *base, const char *path, mode_t mode, int flags);
int fs_fchownat(struct tnode *base, const char *path, uid_t uid, gid_t gid, int flags);
int fs_faccessat(struct tnode *base, const char *path, int mode, int flags);
int fs_fcntl(struct file_descriptor *desc, int command, int arg);
intptr_t fs_mmap(void *addr, size_t length, int prot, int flags, struct file *file, off_t offset);
int fs_rename(const char *old_path, const char *new_path);
ssize_t fs_readlink(const char *path, char *buf, size_t bufsiz);
int fs_symlink(const char *target, const char *linkpath);
int fs_link(const char *oldpath, const char *newpath);
int fs_utimensat(struct tnode *base, const char *path, const struct timespec *times, int flags);
int fs_fstatvfs(struct file *file, struct statvfs *buf);
int fs_statvfs(const char *path, struct statvfs *buf);
int fs_do_mount(struct block_device *device, const char *target, const char *type, unsigned long flags, const void *data);
int fs_mount(const char *source, const char *target, const char *type, unsigned long flags, const void *data);
int fs_umount(const char *target);
struct list_node *fs_file_system_list(void);

intptr_t fs_default_mmap(void *addr, size_t len, int prot, int flags, struct inode *inode, off_t offset);

struct file_descriptor fs_clone(struct file_descriptor desc);
struct file_descriptor fs_dup(struct file_descriptor desc);
struct file_descriptor fs_dup_accross_fork(struct file_descriptor desc);

ssize_t fs_do_read(char *buf, off_t offset, size_t n, const char *source, size_t source_max);

int fs_poll_wait(struct file_state *state, mutex_t *lock, int mask, struct timespec *timeout);
int fs_poll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout);
int fs_select(int nfds, uint8_t *readfds, uint8_t *writefds, uint8_t *exceptfds, const struct timespec *timeout);

int fs_bind_socket_to_inode(struct inode *inode, struct socket *socket);

bool fs_can_read_inode_impl(struct inode *inode, uid_t uid, gid_t gid);
bool fs_can_write_inode_impl(struct inode *inode, uid_t uid, gid_t gid);
bool fs_can_execute_inode_impl(struct inode *inode, uid_t uid, gid_t gid);

bool fs_can_read_inode(struct inode *inode);
bool fs_can_write_inode(struct inode *inode);
bool fs_can_execute_inode(struct inode *inode);
bool fs_is_mount_point(struct inode *inode);

void register_fs(struct file_system *fs);
int fs_mount_initrd(void);
int fs_mount_root(struct fs_root_desc desc);
struct file_system *fs_file_system_from_name(const char *s);

size_t fs_file_size(struct file *file);
char *get_tnode_path(struct tnode *tnode);

struct tnode *fs_get_tnode_for_file(struct file *file);

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
    } else if (S_ISCHR(mode) || S_ISBLK(mode)) {
        return FS_DEVICE;
    } else {
        return 0;
    }
}

static inline struct inode *fs_file_inode(struct file *file) {
    return file->inode;
}

static inline int fs_do_poll(struct wait_queue_entry *entry, int mask, struct file_state *state) {
    int result = state->poll_flags & mask;
    if (result != 0) {
        return result;
    }
    if (entry) {
        spin_lock(&state->queue.lock);
        __wait_queue_enqueue_entry(&state->queue, entry, __func__);
        spin_unlock(&state->queue.lock);
    }
    return 0;
}

static inline void fs_do_poll_finish(struct wait_queue_entry *entry, struct file_state *state) {
    wait_queue_dequeue_entry(&state->queue, entry, __func__);
}

#define fs_for_each_file_system(name) list_for_each_entry(fs_file_system_list(), name, struct file_system, list)

#endif /* _KERNEL_FS_VFS_H */
