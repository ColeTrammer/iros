#ifndef _KERNEL_FS_PROCFS_H
#define _KERNEL_FS_PROCFS_H 1

#include <stdbool.h>
#include <sys/types.h>

struct block_device;
struct fs_device;
struct file;
struct file_system;
struct inode;
struct process;
struct super_block;
struct tnode;

struct procfs_buffer {
    char *buffer;
    size_t size;
};

struct procfs_data {
    struct process *process;
    void *function;
    int fd;
};

typedef struct procfs_buffer (*procfs_file_function_t)(struct procfs_data *data, struct process *process, bool need_buffer);
typedef void (*procfs_directory_function_t)(struct inode *inode, struct process *process, bool loaded);

struct inode *procfs_lookup(struct inode *inode, const char *name);
struct file *procfs_open(struct inode *inode, int flags, int *error);
int procfs_read_all(struct inode *inode, void *buffer);
ssize_t procfs_read(struct file *file, off_t offset, void *buffer, size_t len);
int procfs_mount(struct block_device *device, unsigned long flags, const void *data, struct super_block **super_block_p);

void procfs_register_process(struct process *process);
void procfs_unregister_process(struct process *process);

#endif /* _KERNEL_FS_PROCFS_H */
