#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/fs/cached_dirent.h>
#include <kernel/fs/dev.h>
#include <kernel/fs/file.h>
#include <kernel/fs/file_system.h>
#include <kernel/fs/inode.h>
#include <kernel/fs/inode_store.h>
#include <kernel/fs/super_block.h>
#include <kernel/fs/vfs.h>
#include <kernel/hal/output.h>
#include <kernel/hal/timer.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/mem/vm_region.h>
#include <kernel/proc/task.h>
#include <kernel/time/clock.h>
#include <kernel/util/spinlock.h>

static struct file_system fs;
static struct super_block super_block;

static spinlock_t inode_counter_lock = SPINLOCK_INITIALIZER;
static ino_t inode_counter = 1;

static struct file_system fs = { "dev", 0, &dev_mount, NULL, NULL };

static struct inode_operations dev_i_op = { NULL, &dev_lookup, &dev_open, &dev_stat, &dev_ioctl, NULL,          NULL, NULL,
                                            NULL, NULL,        &dev_mmap, NULL,      NULL,       &dev_read_all, NULL, NULL };

static struct inode_operations dev_dir_i_op = { NULL, &dev_lookup, &dev_open, NULL, NULL, NULL, NULL, NULL,
                                                NULL, NULL,        NULL,      NULL, NULL, NULL, NULL, NULL };

static struct file_operations dev_f_op = { &dev_close, &dev_read, &dev_write, NULL };

static struct file_operations dev_dir_f_op = { NULL, NULL, NULL, NULL };

struct inode *dev_lookup(struct inode *inode, const char *name) {
    if (!inode || !name) {
        return NULL;
    }

    return fs_lookup_in_cache(inode->dirent_cache, name);
}

int dev_read_all(struct inode *inode, void *buf) {
    if (((struct device *) inode->private_data)->ops->read_all) {
        return ((struct device *) inode->private_data)->ops->read_all(inode->private_data, buf);
    }

    return -EOPNOTSUPP;
}

struct file *dev_open(struct inode *inode, int flags, int *error) {
    if (inode->private_data && ((struct device *) inode->private_data)->cannot_open) {
        *error = -EPERM;
        return NULL;
    }

    if (inode->private_data && ((struct device *) inode->private_data)->ops->open) {
        return ((struct device *) inode->private_data)->ops->open(inode->private_data, flags, error);
    }

    struct file *file = calloc(sizeof(struct file), 1);
    file->inode_idenifier = inode->index;
    file->length = inode->size;
    file->start = 0;
    file->position = 0;
    file->f_op = inode->flags & FS_DIR ? &dev_dir_f_op : &dev_f_op;
    file->device = inode->device;
    file->flags = inode->flags;

    if (inode->private_data && ((struct device *) inode->private_data)->ops->on_open) {
        ((struct device *) inode->private_data)->ops->on_open(inode->private_data);
    }

    if (!S_ISBLK(inode->mode) && !S_ISLNK(inode->mode)) {
        file->abilities |= FS_FILE_CANT_SEEK;
    }

    return file;
}

int dev_close(struct file *file) {
    struct inode *inode = fs_inode_get(file->device, file->inode_idenifier);
    assert(inode);

    int error = 0;
    if (((struct device *) inode->private_data)->ops->close) {
        error = ((struct device *) inode->private_data)->ops->close(inode->private_data);
    }

    return error;
}

ssize_t dev_read(struct file *file, off_t offset, void *buffer, size_t len) {
    if (file->flags & FS_DIR) {
        return -EISDIR;
    }

    struct inode *inode = fs_inode_get(file->device, file->inode_idenifier);
    assert(inode);

    if (((struct device *) inode->private_data)->ops->read) {
        inode->access_time = time_read_clock(CLOCK_REALTIME);
        return ((struct device *) inode->private_data)->ops->read(inode->private_data, offset, buffer, len);
    }

    return -EINVAL;
}

ssize_t dev_write(struct file *file, off_t offset, const void *buffer, size_t len) {
    struct inode *inode = fs_inode_get(file->device, file->inode_idenifier);
    assert(inode);

    if (((struct device *) inode->private_data)->ops->write) {
        inode->modify_time = time_read_clock(CLOCK_REALTIME);
        return ((struct device *) inode->private_data)->ops->write(inode->private_data, offset, buffer, len);
    }

    return -EINVAL;
}

int dev_stat(struct inode *inode, struct stat *stat_struct) {
    stat_struct->st_rdev = ((struct device *) inode->private_data)->device_number;
    return 0;
}

int dev_ioctl(struct inode *inode, unsigned long request, void *argp) {
    struct device *device = inode->private_data;

    if (device->ops->ioctl) {
        return device->ops->ioctl(device, request, argp);
    }

    return -ENOTTY;
}

intptr_t dev_mmap(void *addr, size_t len, int prot, int flags, struct inode *inode, off_t offset) {
    struct device *device = inode->private_data;

    if (device->ops->mmap) {
        return device->ops->mmap(device, addr, len, prot, flags, offset);
    }

    return -ENODEV;
}

struct inode *dev_mount(struct file_system *current_fs, char *device_path) {
    assert(current_fs != NULL);
    assert(strlen(device_path) == 0);

    struct inode *root = calloc(1, sizeof(struct inode));

    root->device = 2;
    root->flags = FS_DIR;
    root->i_op = &dev_dir_i_op;
    root->index = inode_counter++;
    init_spinlock(&root->lock);
    root->mode = S_IFDIR | 0777;
    root->mounts = NULL;
    root->private_data = NULL;
    root->size = 0;
    root->super_block = &super_block;
    root->ref_count = 1;
    root->readable = true;
    root->writeable = true;
    root->access_time = root->change_time = root->modify_time = time_read_clock(CLOCK_REALTIME);
    root->dirent_cache = fs_create_dirent_cache();

    super_block.device = root->device;
    super_block.op = NULL;
    super_block.root = root;
    super_block.block_size = PAGE_SIZE;

    current_fs->super_block = &super_block;

    return root;
}

static char *to_dev_path(const char *path) {
    char *new_path = malloc(strlen(path) + 6);
    strcpy(new_path, "/dev/");
    strcat(new_path, path);
    return new_path;
}

void dev_add(struct device *device, const char *_path) {
    char *path = to_dev_path(_path);
    char *_name = strrchr(path, '/');
    *_name = '\0';
    _name++;

    struct tnode *parent;
    int ret = iname(path, 0, &parent);
    if (ret < 0) {
        /* Probably should add the directory that is missing */
        free(path);
        return;
    }

    struct inode *to_add = calloc(1, sizeof(struct inode));
    device->inode = to_add;

    to_add->readable = true;
    to_add->writeable = true;
    to_add->size = 0;

    /* Adds the device */
    device->cannot_open = false;
    if (device->ops->add) {
        device->ops->add(device);
    }

    to_add->device = super_block.device;
    to_add->flags = fs_mode_to_flags(device->type);
    to_add->i_op = &dev_i_op;

    spin_lock(&inode_counter_lock);
    to_add->index = inode_counter++;
    spin_unlock(&inode_counter_lock);

    init_spinlock(&to_add->lock);
    to_add->mode = device->type | 0777;
    to_add->mounts = NULL;
    to_add->private_data = device;
    to_add->ref_count = 1;
    to_add->super_block = &super_block;
    to_add->access_time = to_add->change_time = to_add->modify_time = time_read_clock(CLOCK_REALTIME);

    fs_put_dirent_cache(parent->inode->dirent_cache, to_add, device->name, strlen(device->name));

    free(path);
}

void dev_remove(const char *_path) {
    char *path = to_dev_path(_path);

    struct tnode *tnode;
    int ret = iname(path, 0, &tnode);
    if (ret < 0) {
        /* This probably shouldn't happen */
        free(path);
        return;
    }

    /* Frees the device */
    if (((struct device *) tnode->inode->private_data)->ops->remove) {
        ((struct device *) tnode->inode->private_data)->ops->remove(tnode->inode->private_data);
    }

    free(tnode->inode->private_data);
    fs_del_dirent_cache(tnode->parent->inode->dirent_cache, tnode->name);
    drop_inode_reference(tnode->inode);

    free(path);
}

dev_t dev_get_device_number(struct file *file) {
    struct inode *inode = fs_inode_get(file->device, file->inode_idenifier);
    assert(inode);

    return ((struct device *) inode->private_data)->device_number;
}

void init_dev() {
    load_fs(&fs);
}