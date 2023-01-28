#ifndef _KERNEL_FS_PIPE_H
#define _KERNEL_FS_PIPE_H 1

#include <sys/types.h>
#include <kernel/fs/file.h>
#include <kernel/fs/inode.h>
#include <kernel/proc/wait_queue.h>
#include <kernel/util/ring_buffer.h>

#define PIPE_DEFAULT_BUFFER_SIZE 0x1000
#define PIPE_DEVICE              3

struct pipe_data {
    struct ring_buffer buffer;
    struct wait_queue readers_queue;
    struct wait_queue writers_queue;
    int read_count;
    int write_count;
};

bool is_pipe_read_end_open(struct pipe_data *data);
bool is_pipe_write_end_open(struct pipe_data *data);

struct inode *pipe_new_inode();

struct file *pipe_open(struct inode *inode, int flags, int *error);
ssize_t pipe_read(struct file *file, off_t offset, void *buffer, size_t len);
ssize_t pipe_write(struct file *file, off_t offset, const void *buffer, size_t len);
int pipe_close(struct file *file);
void pipe_clone(struct file *file);
void pipe_all_files_closed(struct inode *inode);

#endif /* _KERNEL_FS_PIPE_H */
