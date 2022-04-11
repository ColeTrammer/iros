#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdatomic.h>
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
#include <kernel/fs/tmp.h>
#include <kernel/fs/vfs.h>
#include <kernel/hal/output.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/mem/vm_region.h>
#include <kernel/proc/task.h>
#include <kernel/time/clock.h>
#include <kernel/util/hash_map.h>
#include <kernel/util/init.h>
#include <kernel/util/spinlock.h>

static struct super_block *devfs_super_block;
static struct hash_map *device_map;

HASH_DEFINE_FUNCTIONS(device, struct fs_device, dev_t, device_number)

struct hash_map *dev_device_hash_map(void) {
    return device_map;
}

struct fs_device *dev_bump_device(struct fs_device *device) {
    atomic_fetch_add(&device->ref_count, 1);
    return device;
}

void dev_drop_device(struct fs_device *device) {
    if (atomic_fetch_sub(&device->ref_count, 1) == 1) {
        if (device->ops->remove) {
            device->ops->remove(device);
        }

        free(device);
    }
}

static struct fs_device_ops null_ops;

struct inode *dev_register(struct fs_device *device) {
    dev_bump_device(device);

    if (!device->ops) {
        device->ops = &null_ops;
    }

    if (device->ops->add) {
        device->ops->add(device);
    }

    assert(!hash_get(device_map, &device->device_number));
    hash_put(device_map, &device->hash);

    struct inode *new_inode = fs_create_inode(devfs_super_block, tmp_get_next_index(), 0, 0, device->mode, 0, &tmp_i_op, NULL);
    new_inode->device_id = device->device_number;
    mutex_lock(&devfs_super_block->root->lock);
    fs_put_dirent_cache(devfs_super_block->root->dirent_cache, new_inode, device->name, strlen(device->name));
    mutex_unlock(&devfs_super_block->root->lock);
    return new_inode;
}

void dev_unregister(struct fs_device *device) {
    hash_del(device_map, &device->device_number);
    dev_drop_device(device);
}

struct fs_device *dev_get_device(dev_t device_number) {
    struct fs_device *device = hash_get_entry(device_map, &device_number, struct fs_device);
    if (device) {
        dev_bump_device(device);
    }
    return device;
}

static int dev_poll(struct file *file, struct wait_queue_entry *entry, int mask) {
    struct fs_device *device = file->private_data;
    assert(device);
    return fs_do_poll(entry, mask, &device->file_state);
}

static void dev_poll_finish(struct file *file, struct wait_queue_entry *entry) {
    struct fs_device *device = file->private_data;
    assert(device);
    fs_do_poll_finish(entry, &device->file_state);
}

static struct file_operations dev_f_op = {
    .close = &dev_close,
    .read = &dev_read,
    .write = &dev_write,
    .poll = &dev_poll,
    .poll_finish = &dev_poll_finish,
};

struct inode *dev_lookup(struct inode *inode, const char *name) {
    if (!inode || !name) {
        return NULL;
    }

    return fs_lookup_in_cache(inode->dirent_cache, name);
}

struct file *dev_open(struct inode *inode, int flags, int *error) {
    struct fs_device *device = dev_get_device(inode->device_id);
    if (!device) {
        *error = -ENXIO;
        return NULL;
    }

    if (device->cannot_open) {
        *error = -EPERM;
        return NULL;
    }

    if (device->ops->open) {
        return device->ops->open(device, flags, error);
    }

    struct file *file =
        fs_create_file(inode, inode->flags, !S_ISBLK(device->mode) ? FS_FILE_CANT_SEEK : 0, flags, &dev_f_op, dev_bump_device(device));
    if (device->ops->on_open) {
        device->ops->on_open(device);
    }

    return file;
}

int dev_close(struct file *file) {
    struct fs_device *device = file->private_data;
    assert(device);

    int error = 0;
    if (device->ops->close) {
        error = device->ops->close(device);
    }

    dev_drop_device(device);
    return error;
}

ssize_t dev_read(struct file *file, off_t offset, void *buffer, size_t len) {
    struct fs_device *device = file->private_data;
    assert(device);

    if (device->ops->read) {
        return device->ops->read(device, offset, buffer, len, !!(file->open_flags & O_NONBLOCK));
    }

    return -EINVAL;
}

ssize_t dev_write(struct file *file, off_t offset, const void *buffer, size_t len) {
    struct fs_device *device = file->private_data;
    assert(device);

    if (device->ops->write) {
        return device->ops->write(device, offset, buffer, len, !!(file->open_flags & O_NONBLOCK));
    }

    return -EINVAL;
}

int dev_ioctl(struct file *file, unsigned long request, void *argp) {
    struct fs_device *device = file->private_data;
    assert(device);

    if (device->ops->ioctl) {
        return device->ops->ioctl(device, request, argp);
    }

    return -ENOTTY;
}

intptr_t dev_mmap(void *addr, size_t len, int prot, int flags, struct file *file, off_t offset) {
    struct fs_device *device = file->private_data;
    assert(device);

    if (device->ops->mmap) {
        return device->ops->mmap(device, addr, len, prot, flags, offset);
    }

    return -ENODEV;
}

uint32_t dev_block_size(struct fs_device *device) {
    assert(device->mode & S_IFBLK);
    assert(device->ops->block_size);
    return device->ops->block_size(device);
}

uint64_t dev_block_count(struct fs_device *device) {
    assert(device->mode & S_IFBLK);
    assert(device->ops->block_count);
    return device->ops->block_count(device);
}

int dev_mount(struct block_device *, unsigned long, const void *, struct super_block **super_block) {
    *super_block = devfs_super_block;
    return 0;
}

int dev_umount(struct super_block *) {
    return 0;
}

static struct file_system devfs = {
    .name = "devfs",
    .mount = dev_mount,
    .umount = dev_umount,
};

static void init_dev() {
    device_map = hash_create_hash_map(device_hash, device_equals, device_key);
    assert(tmp_mount(NULL, 0, NULL, &devfs_super_block) == 0);
    register_fs(&devfs);
}
INIT_FUNCTION(init_dev, fs);
