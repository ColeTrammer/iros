#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/fs/dev.h>
#include <kernel/hal/block.h>
#include <kernel/mem/page.h>
#include <kernel/mem/phys_page.h>
#include <kernel/mem/vm_allocator.h>

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

HASH_DEFINE_FUNCTIONS(page, struct phys_page, off_t, block_offset)

static struct phys_page *block_find_page(struct block_device *block_device, off_t block_offset) {
    assert(block_offset * block_device->block_size % PAGE_SIZE == 0);
    struct phys_page *page = hash_get_entry(block_device->block_hash_map, &block_offset, struct phys_page);
    if (!page) {
        return NULL;
    }
    // Move the page to the front of the LRU list.
    list_remove(&page->lru_list);
    list_prepend(&block_device->lru_list, &page->lru_list);
    return bump_phys_page(page);
}

static struct phys_page *block_put_cache(struct block_device *block_device, struct phys_page *page) {
    size_t num_blocks_cached = hash_size(block_device->block_hash_map);
    // FIXME: instead of using an arbitrary cap, the kernel should only do this when it needs to reclaim
    //        physical memory.
    if (num_blocks_cached >= 10000) {
        struct phys_page *to_remove = list_last_entry(&block_device->lru_list, struct phys_page, lru_list);
        hash_del(block_device->block_hash_map, &to_remove->block_offset);
        list_remove(&to_remove->lru_list);
        drop_phys_page(to_remove);
    }

    list_prepend(&block_device->lru_list, &page->lru_list);
    hash_put(block_device->block_hash_map, &page->hash);
    return bump_phys_page(page);
}

static struct phys_page *block_find_or_read_page(struct block_device *block_device, off_t block_offset) {
    struct phys_page *page = block_find_page(block_device, block_offset);
    if (page) {
        return page;
    }

    page = block_device->op->read_page(block_device, block_offset);
    if (!page) {
        return NULL;
    }
    return block_put_cache(block_device, page);
}

static struct phys_page *block_find_or_empty_page(struct block_device *block_device, off_t block_offset) {
    struct phys_page *page = block_find_page(block_device, block_offset);
    if (page) {
        return page;
    }

    page = allocate_phys_page();
    if (!page) {
        return NULL;
    }
    page->block_offset = block_offset;
    return block_put_cache(block_device, page);
}

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

    blkcnt_t block_step = PAGE_SIZE / block_size;
    off_t block_end = block_offset + block_count;
    off_t block;

    mutex_lock(&device->lock);
    for (block = block_offset; block < block_end;) {
        // This could only be non-zero on the first read, subsequent reads will be page aligned.
        blkcnt_t page_block_offset = block % block_step;
        blkcnt_t blocks_to_read = MIN(block - page_block_offset + block_step, block_end) - block;

        struct phys_page *page = block_find_or_read_page(block_device, block - page_block_offset);
        if (!page) {
            break;
        }

        void *mapped_page = create_phys_addr_mapping(page->phys_addr);
        memcpy(buf + (block - block_offset) * block_size, mapped_page + (page_block_offset * block_size), blocks_to_read * block_size);
        drop_phys_page(page);

        block += blocks_to_read;
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
        // This could only be non-zero on the first write, subsequent writes will be page aligned.
        blkcnt_t page_block_offset = block % block_step;
        blkcnt_t blocks_to_write = MIN(block - page_block_offset + block_step, block_end) - block;

        struct phys_page *page;
        if (page_block_offset == 0 && blocks_to_write == block_step) {
            // There"s no reason to read in the page if the entire thing will be overwritten.
            page = block_find_or_empty_page(block_device, block - page_block_offset);
        } else {
            page = block_find_or_read_page(block_device, block - page_block_offset);
        }

        if (!page) {
            break;
        }

        void *mapped_page = create_phys_addr_mapping(page->phys_addr);
        memcpy(mapped_page + (page_block_offset * block_size), buf + (block - block_offset) * block_size, blocks_to_write * block_size);

        // This should eventually be made asynchronous.
        int ret = block_device->op->sync_page(block_device, page);
        drop_phys_page(page);
        if (ret) {
            break;
        }

        block += blocks_to_write;
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

struct phys_page *block_generic_read_page(struct block_device *self, off_t block_offset) {
    struct phys_page *page = allocate_phys_page();
    if (!page) {
        return NULL;
    }
    page->block_offset = block_offset;

    void *buffer = create_phys_addr_mapping(page->phys_addr);
    blkcnt_t blocks_to_read = PAGE_SIZE / self->block_size;
    if (self->op->read(self, buffer, blocks_to_read, block_offset) != blocks_to_read) {
        drop_phys_page(page);
        return NULL;
    }

    return page;
}

int block_generic_sync_page(struct block_device *self, struct phys_page *page) {
    void *buffer = create_phys_addr_mapping(page->phys_addr);
    blkcnt_t blocks_to_write = PAGE_SIZE / self->block_size;
    if (self->op->write(self, buffer, blocks_to_write, page->block_offset) != blocks_to_write) {
        return -EIO;
    }

    return 0;
}

struct block_device *create_block_device(blkcnt_t block_count, blksize_t block_size, struct block_device_ops *op, void *private_data) {
    struct block_device *block_device = malloc(sizeof(struct block_device));
    block_device->block_count = block_count;
    block_device->block_size = block_size;
    block_device->block_hash_map = hash_create_hash_map(page_hash, page_equals, page_key);
    init_list(&block_device->lru_list);
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
