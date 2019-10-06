#ifndef _KERNEL_FS_PIPE_H
#define _KERNEL_FS_PIPE_H 1

#include <sys/types.h>
#include <kernel/fs/file.h>
#include <kernel/fs/inode.h>

#define PIPE_DEFAULT_BUFFER_SIZE 0x1000
#define PIPE_DEVICE 3

struct pipe_data {
    char *buffer;
    size_t len;
};

struct inode *pipe_new_inode();

struct file *pipe_open(struct inode *inode, int *error);
ssize_t pipe_read(struct file *file, void *buffer, size_t len);
ssize_t pipe_write(struct file *file, const void *buffer, size_t len);
int pipe_close(struct file *file);

void init_pipe();

#endif /* _KERNEL_FS_PIPE_H */