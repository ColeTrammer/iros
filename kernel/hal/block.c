#include <errno.h>
#include <stdlib.h>

#include <kernel/fs/dev.h>
#include <kernel/hal/block.h>
#include <kernel/mem/page.h>

static ssize_t block_read(struct device *device, off_t offset, void *buf, size_t size, bool non_block);
static ssize_t block_write(struct device *device, off_t offset, const void *buf, size_t size, bool non_block);
static blkcnt_t block_block_count(struct device *device);
static blksize_t block_block_size(struct device *device);

static struct device_ops block_device_ops = {
    .read = block_read,
    .write = block_write,
    .block_count = block_block_count,
    .block_size = block_block_size,
};

static ssize_t block_read(struct device *device, off_t offset, void *buf, size_t size, bool non_block) {
    (void) non_block;
    if (offset < 0) {
        return -EINVAL;
    }

    struct block_device *block_device = device->private;
    blksize_t block_size = block_device->block_size;
    if (size % block_size != 0 || offset % block_size != 0) {
        return -ENXIO;
    }

    blkcnt_t block_count = size / block_size;
    off_t block_offset = offset / block_size;
    if (block_count + block_offset > block_device->block_count) {
        return -ENXIO;
    }

    // Write out blocks in page size multiples. This could be improved, but at least allows for efficent DMA use.
    blkcnt_t block_step = PAGE_SIZE / block_size;
    off_t block_end = block_offset + block_count;
    off_t block;

    mutex_lock(&device->lock);
    for (block = block_offset; block < block_end;) {
        blkcnt_t blocks_to_write = block_end - block < block_step ? block_end - block : block_step;
        blkcnt_t blocks_written = block_device->op->read(block_device, buf + (block - block_offset) * block_size, blocks_to_write, block);
        block += blocks_written;
        if (blocks_written != blocks_to_write) {
            break;
        }
    }
    mutex_unlock(&device->lock);

    ssize_t ret = (block - block_offset) * block_size;
    if (ret == 0) {
        return -EIO;
    }
    return ret;
}

static ssize_t block_write(struct device *device, off_t offset, const void *buf, size_t size, bool non_block) {
    (void) non_block;
    if (offset < 0) {
        return -EINVAL;
    }

    struct block_device *block_device = device->private;
    blksize_t block_size = block_device->block_size;
    if (size % block_size != 0 || offset % block_size != 0) {
        return -ENXIO;
    }

    blkcnt_t block_count = size / block_size;
    off_t block_offset = offset / block_size;
    if (block_count + block_offset > block_device->block_count) {
        return -ENXIO;
    }

    // Write out blocks in page size multiples. This could be improved, but at least allows for efficent DMA use.
    blkcnt_t block_step = PAGE_SIZE / block_size;
    off_t block_end = block_offset + block_count;
    off_t block;

    mutex_lock(&device->lock);
    for (block = block_offset; block < block_end;) {
        blkcnt_t blocks_to_write = block_end - block < block_step ? block_end - block : block_step;
        blkcnt_t blocks_written = block_device->op->write(block_device, buf + (block - block_offset) * block_size, blocks_to_write, block);
        block += blocks_written;
        if (blocks_written != blocks_to_write) {
            break;
        }
    }
    mutex_unlock(&device->lock);

    ssize_t ret = (block - block_offset) * block_size;
    if (ret == 0) {
        return -EIO;
    }
    return ret;
}

static blkcnt_t block_block_count(struct device *device) {
    struct block_device *block_device = device->private;
    return block_device->block_count;
}

static blksize_t block_block_size(struct device *device) {
    struct block_device *block_device = device->private;
    return block_device->block_size;
}

struct block_device *create_block_device(blkcnt_t block_count, blksize_t block_size, struct block_device_ops *op, void *private_data) {
    struct block_device *block_device = malloc(sizeof(struct block_device));
    block_device->block_count = block_count;
    block_device->block_size = block_size;
    block_device->op = op;
    block_device->private_data = private_data;
    return block_device;
}

void block_register_device(struct block_device *block_device, dev_t device_number) {
    struct device *device = calloc(1, sizeof(struct device));
    device->device_number = device_number;
    device->type = S_IFBLK;
    device->readable = device->writeable = true;
    init_mutex(&device->lock);
    device->ops = &block_device_ops;
    device->private = block_device;
    dev_register(device);
}
