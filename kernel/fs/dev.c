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
#include <kernel/fs/super_block.h>
#include <kernel/fs/vfs.h>
#include <kernel/hal/output.h>
#include <kernel/hal/timer.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/mem/vm_region.h>
#include <kernel/proc/task.h>
#include <kernel/time/clock.h>
#include <kernel/util/hash_map.h>
#include <kernel/util/spinlock.h>

static struct hash_map *device_map;

HASH_DEFINE_FUNCTIONS(device, struct device, dev_t, device_number)

void dev_register(struct device *device) {
    assert(!hash_get(device_map, &device->device_number));
    hash_put(device_map, device);
}

void dev_unregister(struct device *device) {
    hash_del(device_map, &device->device_number);

    if (device->ops->remove) {
        device->ops->remove(device);
    }
}

struct device *dev_get_device(dev_t device_number) {
    return hash_get(device_map, &device_number);
}

static struct file_operations dev_f_op = { &dev_close, &dev_read, &dev_write, NULL };

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
    file->position = 0;
    file->f_op = &dev_f_op;
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
    struct inode *inode = fs_file_inode(file);
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

    struct inode *inode = fs_file_inode(file);
    assert(inode);

    if (((struct device *) inode->private_data)->ops->read) {
        inode->access_time = time_read_clock(CLOCK_REALTIME);
        return ((struct device *) inode->private_data)->ops->read(inode->private_data, offset, buffer, len);
    }

    debug_log("???\n");
    return -EINVAL;
}

ssize_t dev_write(struct file *file, off_t offset, const void *buffer, size_t len) {
    struct inode *inode = fs_file_inode(file);
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

void init_dev() {
    device_map = hash_create_hash_map(device_hash, device_equals, device_key);
}
