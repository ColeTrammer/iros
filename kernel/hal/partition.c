#include <kernel/fs/dev.h>
#include <kernel/fs/vfs.h>
#include <kernel/hal/block.h>
#include <kernel/hal/mbr.h>
#include <kernel/hal/partition.h>
#include <kernel/mem/phys_page.h>

static blkcnt_t partition_read(struct block_device *self, void *buf, blkcnt_t block_count, off_t block_offset) {
    struct block_device *root = self->private_data;
    mutex_lock(&root->device->lock);
    blkcnt_t ret = root->op->read(root, buf, block_count, self->partition_offset + block_offset);
    mutex_unlock(&root->device->lock);
    return ret;
}

static blkcnt_t partition_write(struct block_device *self, const void *buf, blkcnt_t block_count, off_t block_offset) {
    struct block_device *root = self->private_data;
    mutex_lock(&root->device->lock);
    blkcnt_t ret = root->op->write(root, buf, block_count, self->partition_offset + block_offset);
    mutex_unlock(&root->device->lock);
    return ret;
}

static struct phys_page *partition_read_page(struct block_device *self, off_t block_offset) {
    struct block_device *root = self->private_data;
    mutex_lock(&root->device->lock);
    struct phys_page *ret = root->op->read_page(root, self->partition_offset + block_offset);
    mutex_unlock(&root->device->lock);
    return ret;
}

static int partition_sync_page(struct block_device *self, struct phys_page *page) {
    struct block_device *root = self->private_data;
    mutex_lock(&root->device->lock);
    int ret = root->op->sync_page(root, page);
    mutex_unlock(&root->device->lock);
    return ret;
}

static struct block_device_ops partition_ops = {
    .read = partition_read,
    .write = partition_write,
    .read_page = partition_read_page,
    .sync_page = partition_sync_page,
};

struct block_device *create_and_register_partition_device(struct block_device *root_device, blkcnt_t block_count, off_t partition_offset,
                                                          int partition_number, struct block_device_info info) {
    struct block_device *block_device = create_block_device(block_count, root_device->block_size, info, &partition_ops, root_device);
    block_device->partition_offset = partition_offset;
    block_device->partition_number = partition_number;

    char name[16];
    snprintf(name, sizeof(name) - 1, "%s%d", root_device->device->name, partition_number - 1);
    block_register_device(block_device, name, root_device->device->device_number + partition_number);

    mutex_unlock(&root_device->device->lock);
    fs_for_each_file_system(fs) {
        if (fs_id_matches_file_system(info.filesystem_type_id, fs)) {
            if (!fs->determine_fsid(block_device, &block_device->info.filesystem_id)) {
                goto done;
            }
        }
    }

done:
    mutex_lock(&root_device->device->lock);
    return block_device;
}

void block_partition_device(struct block_device *block_device) {
    mbr_partition_device(block_device);
}
