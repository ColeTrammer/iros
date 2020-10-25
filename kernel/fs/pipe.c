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
#include <kernel/hal/processor.h>
#include <kernel/hal/timer.h>
#include <kernel/sched/task_sched.h>
#include <kernel/time/clock.h>
#include <kernel/util/spinlock.h>

// #define PIPE_DEBUG

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

#ifdef PIPE_DEBUG
    debug_log("Created pipe: [ %llu ]\n", inode->index);
#endif /* PIPE_DEBUG */
    return inode;
}

struct file *pipe_open(struct inode *inode, int flags, int *error) {
    struct file *file = fs_create_file(inode, FS_FIFO, FS_FILE_CANT_SEEK, flags, &pipe_f_op, NULL);

    struct pipe_data *data = inode->pipe_data;
    mutex_lock(&inode->lock);
    if (!data) {
        data = malloc(sizeof(struct pipe_data));
        init_ring_buffer(&data->buffer, PIPE_DEFAULT_BUFFER_SIZE);
        init_wait_queue(&data->readers_queue);
        init_wait_queue(&data->writers_queue);
        data->read_count = 0;
        data->write_count = 0;
        inode->pipe_data = data;
    }

    if (file->abilities & FS_FILE_CAN_WRITE) {
        if (ring_buffer_dead(&data->buffer)) {
            init_ring_buffer(&data->buffer, PIPE_DEFAULT_BUFFER_SIZE);
        }

        if (data->write_count++ == 0) {
            fs_set_state_bit(&inode->file_state, POLLIN, !ring_buffer_empty(&data->buffer));
            wake_up_all(&data->writers_queue);
        }
    }

    if (file->abilities & FS_FILE_CAN_READ) {
        if (data->read_count++ == 0) {
            fs_set_state_bit(&inode->file_state, POLLOUT, !ring_buffer_full(&data->buffer));
            wake_up_all(&data->readers_queue);
        }
    }

    if ((file->abilities & FS_FILE_CAN_WRITE) && data->read_count == 0) {
        if (flags & O_NONBLOCK) {
            mutex_unlock(&inode->lock);
            *error = -ENXIO;
            fs_close(file);
            return NULL;
        } else {
            int ret = wait_for_with_mutex_interruptible(get_current_task(), data->read_count > 0, &data->readers_queue, &inode->lock);
            if (ret) {
                *error = ret;
                fs_close(file);
                return NULL;
            }
        }
    }

    if ((file->abilities & FS_FILE_CAN_READ) && data->write_count == 0) {
        if (!(flags & O_NONBLOCK)) {
            int ret = wait_for_with_mutex_interruptible(get_current_task(), data->write_count > 0, &data->writers_queue, &inode->lock);
            if (ret) {
                *error = ret;
                fs_close(file);
                return NULL;
            }
        }
    }
    mutex_unlock(&inode->lock);

    *error = 0;
    return file;
}

ssize_t pipe_read(struct file *file, off_t offset, void *buffer, size_t _len) {
    assert(offset == 0);

#ifdef PIPE_DEBUG
    debug_log("Reading from pipe: [ %lu ]\n", _len);
#endif /* PIPE_DEBUG */

    struct inode *inode = fs_file_inode(file);
    assert(inode);
    struct pipe_data *data = inode->pipe_data;
    assert(data);

    mutex_lock(&inode->lock);
again:;
    size_t len = MIN(_len, ring_buffer_size(&data->buffer));
    if (len == 0 && is_pipe_write_end_open(data)) {
        if (file->open_flags & O_NONBLOCK) {
            mutex_unlock(&inode->lock);
            return -EAGAIN;
        }

        int ret = inode_poll_wait(inode, POLLIN, NULL);
        if (ret) {
            return ret;
        }
        goto again;
    }

    if (len != 0) {
        ring_buffer_user_read(&data->buffer, buffer, len);

        fs_trigger_state(&inode->file_state, POLLOUT);
        fs_set_state_bit(&inode->file_state, POLLIN, !ring_buffer_empty(&data->buffer));
    }

    mutex_unlock(&inode->lock);
    return len;
}

ssize_t pipe_write(struct file *file, off_t offset, const void *buffer, size_t len) {
    assert(offset == 0);

#ifdef PIPE_DEBUG
    debug_log("Writing to pipe: [ %lu ]\n", len);
#endif /* PIPE_DEBUG */

    struct inode *inode = fs_file_inode(file);
    assert(inode);
    struct pipe_data *data = inode->pipe_data;
    assert(data);

    mutex_lock(&inode->lock);

    if (!is_pipe_read_end_open(data)) {
        mutex_unlock(&inode->lock);
        signal_task(get_current_task()->process->pid, get_current_task()->tid, SIGPIPE);
        return -EPIPE;
    }

    size_t buffer_index = 0;
    while (buffer_index < len) {
        size_t space_available = ring_buffer_space(&data->buffer);
        if (!space_available) {
            if (file->open_flags & O_NONBLOCK) {
                mutex_unlock(&inode->lock);
                return buffer_index == 0 ? -EAGAIN : (ssize_t) buffer_index;
            }
            int ret = inode_poll_wait(inode, POLLOUT, NULL);
            if (ret) {
                return ret;
            }
            continue;
        }

        size_t amount_to_write = MIN(space_available, len - buffer_index);
        ring_buffer_user_write(&data->buffer, buffer + buffer_index, amount_to_write);
        buffer_index += amount_to_write;
        fs_trigger_state(&inode->file_state, POLLIN);
        fs_set_state_bit(&inode->file_state, POLLOUT, !ring_buffer_full(&data->buffer));
        inode->modify_time = time_read_clock(CLOCK_REALTIME);
    }

    mutex_unlock(&inode->lock);
    return buffer_index;
}

static void free_pipe_data(struct inode *inode) {
#ifdef PIPE_DEBUG
    debug_log("Destroying pipe: [ %llu ]\n", inode->index);
#endif /* PIPE_DEBUG */

    struct pipe_data *data = inode->pipe_data;
    inode->pipe_data = NULL;

    if (data) {
        kill_ring_buffer(&data->buffer);
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
            fs_trigger_state(&inode->file_state, POLLIN);
        }
    }
    if (file->abilities & FS_FILE_CAN_READ) {
        if (data->read_count-- == 1) {
            // The reader disconnected, wake up anyone waiting to write to this pipe.
            fs_trigger_state(&inode->file_state, POLLOUT);
        }
    }
    mutex_unlock(&inode->lock);

    return 0;
}

void pipe_all_files_closed(struct inode *inode) {
    free_pipe_data(inode);

    fs_set_state(&inode->file_state, 0);

    // Drop our inode reference so that the anonymous pipe inode is automatically deleted.
    if (inode->fsid == PIPE_DEVICE) {
        drop_inode_reference(inode);
    }
}
