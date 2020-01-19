#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <kernel/fs/file.h>
#include <kernel/fs/inode.h>
#include <kernel/fs/inode_store.h>
#include <kernel/fs/pipe.h>
#include <kernel/hal/timer.h>
#include <kernel/sched/task_sched.h>
#include <kernel/util/spinlock.h>

static spinlock_t pipe_index_lock = SPINLOCK_INITIALIZER;
static ino_t pipe_index = 1;

static struct inode_operations pipe_i_op = { NULL, NULL, &pipe_open, NULL, NULL,
                                             NULL, NULL, NULL,       NULL, NULL,
                                             NULL, NULL, NULL, NULL,       NULL, &pipe_on_inode_destruction };

static struct file_operations pipe_f_op = { &pipe_close, &pipe_read, &pipe_write, &pipe_clone };

bool is_pipe_write_end_open(struct inode *inode) {
    struct pipe_data *data = inode->private_data;
    return data->write_count > 0;
}

struct inode *pipe_new_inode() {
    struct inode *inode = calloc(1, sizeof(struct inode));
    inode->device = PIPE_DEVICE;
    inode->flags = FS_FIFO;
    inode->i_op = &pipe_i_op;
    spin_lock(&pipe_index_lock);
    inode->index = pipe_index++;
    spin_unlock(&pipe_index_lock);
    init_spinlock(&inode->lock);
    inode->mode = 0777 | S_IFIFO;
    inode->mounts = NULL;
    inode->parent = NULL;
    inode->writeable = true;
    inode->readable = false;
    inode->access_time = inode->change_time = inode->modify_time = get_time_as_timespec();

    debug_log("Creating pipe: [ %llu ]\n", inode->index);

    struct pipe_data *pipe_data = malloc(sizeof(struct pipe_data));
    pipe_data->buffer = malloc(PIPE_DEFAULT_BUFFER_SIZE);
    pipe_data->len = PIPE_DEFAULT_BUFFER_SIZE;
    pipe_data->write_count = 0;
    inode->private_data = pipe_data;

    inode->ref_count = 0;
    inode->size = 0;
    inode->super_block = NULL;
    inode->tnode_list = NULL;

    return inode;
}

struct file *pipe_open(struct inode *inode, int flags, int *error) {
    assert(!(flags & O_RDWR));

    struct file *file = calloc(1, sizeof(struct file));
    file->device = inode->device;
    file->f_op = &pipe_f_op;
    file->flags = inode->flags;
    file->inode_idenifier = inode->index;
    file->length = inode->size;
    file->position = 0;
    file->start = 0;
    file->abilities = 0;
    file->ref_count = 0;

    struct pipe_data *data = inode->private_data;

    spin_lock(&inode->lock);
    inode->ref_count++;
    if (flags & O_WRONLY) {
        data->write_count++;
    }
    spin_unlock(&inode->lock);

    *error = 0;
    return file;
}

ssize_t pipe_read(struct file *file, void *buffer, size_t _len) {
    debug_log("Reading from pipe: [ %llu, %lu, %lu ]\n", file->inode_idenifier, _len, file->position);

    struct inode *inode = fs_inode_get(file->device, file->inode_idenifier);
    assert(inode);
    struct pipe_data *data = inode->private_data;
    assert(data);

    size_t len = MIN(_len, inode->size - file->position);
    if (len == 0 && is_pipe_write_end_open(inode)) {
        proc_block_until_pipe_is_readable(get_current_task(), inode);
        len = MIN(_len, inode->size - file->position);
    }

    spin_lock(&inode->lock);
    memcpy(buffer, data->buffer + file->position, len);
    file->position += len;

    if (file->position == inode->size) {
        inode->readable = false;
    }

    spin_unlock(&inode->lock);
    return len;
}

ssize_t pipe_write(struct file *file, const void *buffer, size_t len) {
    debug_log("Writing to pipe: [ %llu, %lu, %lu ]\n", file->inode_idenifier, len, file->position);

    struct inode *inode = fs_inode_get(file->device, file->inode_idenifier);
    assert(inode);
    assert(file->position == inode->size);
    struct pipe_data *data = inode->private_data;
    assert(data);

    spin_lock(&inode->lock);

    if (data->len < file->position + len) {
        data->len = MAX(data->len * 2, file->position + len);
        data->buffer = realloc(data->buffer, data->len);
    }

    // Now there is something to read from
    inode->readable = true;

    memcpy(data->buffer + file->position, buffer, len);

    inode->size += len;
    file->position += len;

    inode->modify_time = get_time_as_timespec();

    spin_unlock(&inode->lock);

    return len;
}

int pipe_close(struct file *file) {
    struct inode *inode = fs_inode_get(file->device, file->inode_idenifier);
    // This means we already killed the inode and called on_inode_destruction
    // We still needs this method to detect when there are no more writers
    // to the pipe, but obviously if no one is using the pipe it no longer
    // matters
    if (!inode) {
        return 0;
    }

    struct pipe_data *data = inode->private_data;

    spin_lock(&inode->lock);
    if (file->abilities & FS_FILE_CAN_WRITE) {
        data->write_count--;
    }

    spin_unlock(&inode->lock);
    return 0;
}

void pipe_on_inode_destruction(struct inode *inode) {
    debug_log("Destroying pipe: [ %llu ]\n", inode->index);

    struct pipe_data *data = inode->private_data;
    if (data) {
        free(data->buffer);
        free(data);
    }
}

void pipe_clone(struct file *file) {
    if (file->abilities & FS_FILE_CAN_WRITE) {
        struct inode *inode = fs_inode_get(file->device, file->inode_idenifier);
        struct pipe_data *data = inode->private_data;

        spin_lock(&inode->lock);
        data->write_count++;
        spin_unlock(&inode->lock);
    }
}

void init_pipe() {
    fs_inode_create_store(PIPE_DEVICE);
}