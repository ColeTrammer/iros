#include <assert.h>
#include <errno.h>
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
#include <kernel/fs/pipe.h>
#include <kernel/fs/vfs.h>
#include <kernel/hal/timer.h>
#include <kernel/sched/task_sched.h>
#include <kernel/time/clock.h>
#include <kernel/util/spinlock.h>

static spinlock_t pipe_index_lock = SPINLOCK_INITIALIZER;
static ino_t pipe_index = 1;

static struct inode_operations pipe_i_op = { .open = &pipe_open };

static struct file_operations pipe_f_op = { .close = &pipe_close, .read = &pipe_read, .write = &pipe_write };

bool is_pipe_read_end_open(struct pipe_data *data) {
    return data->read_count > 0;
}

bool is_pipe_write_end_open(struct pipe_data *data) {
    return data->write_count > 0;
}

struct inode *pipe_new_inode() {
    spin_lock(&pipe_index_lock);
    ino_t id = pipe_index++;
    spin_unlock(&pipe_index_lock);

    struct inode *inode = fs_create_inode_without_sb(PIPE_DEVICE, id, get_current_task()->process->euid, get_current_task()->process->egid,
                                                     0777 | S_IFIFO, 0, &pipe_i_op, NULL);

    debug_log("Created pipe: [ %llu ]\n", inode->index);
    return inode;
}

struct file *pipe_open(struct inode *inode, int flags, int *error) {
    struct file *file = fs_create_file(inode, FS_FIFO, FS_FILE_CANT_SEEK, flags, &pipe_f_op, NULL);

    struct pipe_data *data = inode->pipe_data;
    mutex_lock(&inode->lock);
    if (!data) {
        data = malloc(sizeof(struct pipe_data));
        data->buffer = NULL;
        data->len = 0;
        data->max = PIPE_DEFAULT_BUFFER_SIZE;
        data->read_count = 0;
        data->write_count = 0;
        inode->pipe_data = data;
    }

    if (file->abilities & FS_FILE_CAN_WRITE) {
        if (data->buffer == NULL) {
            data->buffer = malloc(data->max);
        }

        if (data->write_count++ == 0) {
            // Clear the exceptional_activity flag once another writer is opened.
            inode->excetional_activity = false;
        }
    }

    if (file->abilities & FS_FILE_CAN_READ) {
        if (data->read_count++ == 0) {
            // Clear the exceptional_activity flag once another reader is opened.
            inode->excetional_activity = false;
        }
    }

    if ((file->abilities & FS_FILE_CAN_WRITE) && data->read_count == 0) {
        if (flags & O_NONBLOCK) {
            mutex_unlock(&inode->lock);
            *error = -ENXIO;
            fs_close(file);
            return NULL;
        } else {
            mutex_unlock(&inode->lock);

            int ret = proc_block_until_pipe_has_readers(get_current_task(), data);
            if (ret) {
                *error = ret;
                fs_close(file);
                return NULL;
            }

            mutex_lock(&inode->lock);
        }
    }

    if ((file->abilities & FS_FILE_CAN_READ) && data->write_count == 0) {
        if (!(flags & O_NONBLOCK)) {
            mutex_unlock(&inode->lock);

            int ret = proc_block_until_pipe_has_writers(get_current_task(), data);
            if (ret) {
                mutex_unlock(&inode->lock);
                *error = ret;
                fs_close(file);
                return NULL;
            }

            mutex_lock(&inode->lock);
        }
    }
    mutex_unlock(&inode->lock);

    *error = 0;
    return file;
}

ssize_t pipe_read(struct file *file, off_t offset, void *buffer, size_t _len) {
    assert(offset == 0);

    debug_log("Reading from pipe: [ %lu, %lu ]\n", _len, file->position);

    struct inode *inode = fs_file_inode(file);
    assert(inode);
    struct pipe_data *data = inode->pipe_data;
    assert(data);

    size_t len = MIN(_len, data->len - file->position);
    if (len == 0 && (is_pipe_write_end_open(data) || inode->fsid != PIPE_DEVICE)) {
        int ret = proc_block_until_inode_is_readable(get_current_task(), inode);
        if (ret) {
            return ret;
        }
        len = MIN(_len, data->len - file->position);
    }

    mutex_lock(&inode->lock);
    memcpy(buffer, data->buffer + file->position, len);
    file->position += len;

    if (file->position == (off_t) data->len) {
        inode->readable = false;
    }

    mutex_unlock(&inode->lock);
    return len;
}

ssize_t pipe_write(struct file *file, off_t offset, const void *buffer, size_t len) {
    assert(offset == 0);
    debug_log("Writing to pipe: [ %lu, %lu ]\n", len, file->position);

    struct inode *inode = fs_file_inode(file);
    assert(inode);
    struct pipe_data *data = inode->pipe_data;
    assert(file->position == (off_t) data->len);
    assert(data);

    mutex_lock(&inode->lock);

    if (!is_pipe_read_end_open(data)) {
        mutex_unlock(&inode->lock);
        signal_task(get_current_task()->process->pid, get_current_task()->tid, SIGPIPE);
        return -EPIPE;
    }

    if (data->max < file->position + len) {
        data->max = MAX(data->max * 2, file->position + len);
        data->buffer = realloc(data->buffer, data->max);
    }

    // Now there is something to read from
    inode->readable = true;

    memcpy(data->buffer + file->position, buffer, len);

    data->len += len;
    file->position += len;

    inode->modify_time = time_read_clock(CLOCK_REALTIME);

    mutex_unlock(&inode->lock);

    return len;
}

static void free_pipe_data(struct inode *inode) {
    debug_log("Destroying pipe: [ %llu ]\n", inode->index);

    struct pipe_data *data = inode->pipe_data;
    inode->pipe_data = NULL;

    if (data) {
        free(data->buffer);
        free(data);
    }
}

int pipe_close(struct file *file) {
    struct inode *inode = fs_file_inode(file);
    // This means we already killed the inode and called on_inode_destruction
    // We still needs this method to detect when there are no more writers
    // to the pipe, but obviously if no one is using the pipe it no longer
    // matters
    if (!inode) {
        return 0;
    }

    struct pipe_data *data = inode->pipe_data;

    mutex_lock(&inode->lock);
    if (file->abilities & FS_FILE_CAN_WRITE) {
        if (data->write_count-- == 1) {
            // The writer disconnected, wake up anyone waiting to read from this pipe.
            inode->excetional_activity = true;
        }
    }
    if (file->abilities & FS_FILE_CAN_READ) {
        if (data->read_count-- == 1) {
            // The reader disconnected, wake up anyone waiting to write to this pipe.
            inode->excetional_activity = true;
        }
    }
    mutex_unlock(&inode->lock);

    return 0;
}

void pipe_all_files_closed(struct inode *inode) {
    free_pipe_data(inode);

    inode->readable = inode->writeable = inode->excetional_activity = false;

    // Drop our inode reference so that the anonymous pipe inode is automatically deleted.
    if (inode->fsid == PIPE_DEVICE) {
        drop_inode_reference(inode);
    }
}

void init_pipe() {}
