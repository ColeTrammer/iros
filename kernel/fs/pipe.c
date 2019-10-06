#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stddef.h>
#include <sys/param.h>
#include <assert.h>

#include <kernel/fs/pipe.h>
#include <kernel/fs/inode.h>
#include <kernel/fs/inode_store.h>
#include <kernel/fs/file.h>
#include <kernel/util/spinlock.h>

static spinlock_t pipe_index_lock = SPINLOCK_INITIALIZER;
static ino_t pipe_index = 1;

static struct inode_operations pipe_i_op = {
    NULL, NULL, &pipe_open, NULL, NULL, NULL
};

static struct file_operations pipe_f_op = {
    &pipe_close, &pipe_read, &pipe_write
};

struct inode *pipe_new_inode() {
    struct inode *inode = malloc(sizeof(struct inode));
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

    debug_log("Creating pipe: [ %llu ]\n", inode->index);

    struct pipe_data *pipe_data = malloc(sizeof(struct pipe_data));
    pipe_data->buffer = malloc(PIPE_DEFAULT_BUFFER_SIZE);
    pipe_data->len = PIPE_DEFAULT_BUFFER_SIZE;
    inode->private_data = pipe_data;

    inode->ref_count = 0;
    inode->size = 0;
    inode->super_block = NULL;
    inode->tnode_list = NULL;

    return inode;
}

struct file *pipe_open(struct inode *inode, int *error) {
    struct file *file = malloc(sizeof(struct file));
    file->device = inode->device;
    file->f_op = &pipe_f_op;
    file->flags = inode->flags;
    file->inode_idenifier = inode->index;
    file->length = inode->size;
    file->position = 0;
    file->start = 0;

    spin_lock(&inode->lock);
    inode->ref_count++;
    spin_unlock(&inode->lock);

    *error = 0;
    return file;
}

ssize_t pipe_read(struct file *file, void *buffer, size_t len) {
    debug_log("Reading from pipe: [ %llu, %lu, %lu ]\n", file->inode_idenifier, len, file->position);

    struct inode *inode = fs_inode_get(file->device, file->inode_idenifier);
    assert(inode);
    struct pipe_data *data = inode->private_data;
    assert(data);

    len = MIN(len, inode->size - file->position);

    spin_lock(&inode->lock);
    memcpy(buffer, data->buffer + file->position, len);
    spin_unlock(&inode->lock);

    file->position += len;
    return len;
}

ssize_t pipe_write(struct file *file, const void *buffer, size_t len) {
    debug_log("Writing to pipe: [ %llu, %lu, %lu ]\n", file->inode_idenifier, len, file->position);

    struct inode *inode = fs_inode_get(file->device, file->inode_idenifier);
    assert(inode);
    assert(file->position == inode->size);
    struct pipe_data *data = inode->private_data;
    assert(data);

    if (data->len < file->position + len) {
        data->len = MAX(data->len * 2, file->position + len);
        data->buffer = realloc(data->buffer, data->len);
    }

    spin_lock(&inode->lock);
    memcpy(data->buffer + file->position, buffer, len);

    inode->size += len;
    spin_unlock(&inode->lock);

    file->position += len;
    return len;
}

int pipe_close(struct file *file) {
    struct inode *inode = fs_inode_get(file->device, file->inode_idenifier);
    assert(inode);

    spin_lock(&inode->lock);
    inode->ref_count--;
    if (inode->ref_count == 0) {
        debug_log("Destroying pipe: [ %llu ]\n", inode->index);

        struct pipe_data *data = inode->private_data;
        free(data->buffer);
        free(data);
        spin_unlock(&inode->lock);
        fs_inode_del(inode->device, inode->index);
        free(inode);
        return 0;
    }

    spin_unlock(&inode->lock);
    return 0;
}

void init_pipe() {
    fs_inode_create_store(PIPE_DEVICE);
}