#ifndef _KERNEL_FS_PROCFS_H
#define _KERNEL_FS_PROCFS_H 1

#include <sys/types.h>

struct file;
struct file_system;
struct inode;
struct process;

struct procfs_buffer {
    char *buffer;
    size_t size;
};

typedef struct procfs_buffer (*procfs_function_t)(struct process *process);

struct tnode *procfs_lookup(struct inode *inode, const char *name);
struct file *procfs_open(struct inode *inode, int flags, int *error);
ssize_t procfs_read(struct file *file, off_t offset, void *buffer, size_t len);
struct tnode *procfs_mount(struct file_system *fs, char *device_path);

void procfs_register_process(struct process *process);
void procfs_unregister_process(struct process *process);

void init_procfs();

#endif /* _KERNEL_FS_PROCFS_H */