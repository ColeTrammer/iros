#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/fs/dev.h>
#include <kernel/hal/block.h>
#include <kernel/hal/partition.h>
#include <kernel/mem/page.h>
#include <kernel/mem/page_frame_allocator.h>
#include <kernel/mem/phys_page.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/proc/stats.h>

static ssize_t block_read(struct fs_device *device, off_t offset, void *buf, size_t size, bool non_block);
static ssize_t block_write(struct fs_device *device, off_t offset, const void *buf, size_t size, bool non_block);
static uint64_t block_block_count(struct fs_device *device);
static uint32_t block_block_size(struct fs_device *device);

static struct fs_device_ops block_device_ops = {
    .read = block_read,
    .write = block_write,
    .block_count = block_block_count,
    .block_size = block_block_size,
};

static struct list_node block_devices = INIT_LIST(block_devices);

HASH_DEFINE_FUNCTIONS(page, struct phys_page, off_t, block_offset)

static struct phys_page *block_find_page(struct block_device *block_device, off_t block_offset) {
    block_offset += block_device->partition_offset;
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

static void block_shrink_cache(struct block_device *block_device) {
    struct phys_page *to_remove = list_last_entry(&block_device->lru_list, struct phys_page, lru_list);
    hash_del(block_device->block_hash_map, &to_remove->block_offset);
    list_remove(&to_remove->lru_list);
    drop_phys_page(to_remove);
}

static struct phys_page *block_put_cache(struct block_device *block_device, struct phys_page *page) {
    // If less than 10% of memory is available, don't add new cache items (by removing old ones).
    if (g_phys_page_stats.phys_memory_total - g_phys_page_stats.phys_memory_allocated < g_phys_page_stats.phys_memory_total / 10) {
        block_shrink_cache(block_device);
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

    page = block_allocate_phys_page(block_device);
    if (!page) {
        return NULL;
    }
    page->block_offset = block_offset + block_device->partition_offset;
    return block_put_cache(block_device, page);
}

static ssize_t block_read(struct fs_device *device, off_t offset, void *buf, size_t size, bool non_block) {
    (void) non_block;
    if (offset < 0) {
        return -EINVAL;
    }

    struct block_device *block_device = device->private;
    uint32_t block_size = block_device->block_size;
    if (size % block_size != 0 || offset % block_size != 0) {
        return -ENXIO;
    }

    uint64_t block_count = size / block_size;
    off_t block_offset = offset / block_size;
    if (block_count + block_offset > block_device->block_count) {
        return -ENXIO;
    }

    uint64_t block_step = PAGE_SIZE / block_size;
    off_t block_end = block_offset + block_count;
    off_t block;

    mutex_lock(&device->lock);
    for (block = block_offset; block < block_end;) {
        // This could only be non-zero on the first read, subsequent reads will be page aligned.
        uint64_t page_block_offset = block % block_step;
        uint64_t blocks_to_read = MIN(block - page_block_offset + block_step, (uint64_t) block_end) - block;

        struct phys_page *page = block_find_or_read_page(block_device, block - page_block_offset);
        if (!page) {
            break;
        }

        void *mapped_page = create_phys_addr_mapping(page->phys_addr);
        memcpy(buf + (block - block_offset) * block_size, mapped_page + (page_block_offset * block_size), blocks_to_read * block_size);
        free_phys_addr_mapping(mapped_page);
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

static ssize_t block_write(struct fs_device *device, off_t offset, const void *buf, size_t size, bool non_block) {
    (void) non_block;
    if (offset < 0) {
        return -EINVAL;
    }

    struct block_device *block_device = device->private;
    uint32_t block_size = block_device->block_size;
    if (size % block_size != 0 || offset % block_size != 0) {
        return -ENXIO;
    }

    uint64_t block_count = size / block_size;
    off_t block_offset = offset / block_size;
    if (block_count + block_offset > block_device->block_count) {
        return -ENXIO;
    }

    // Write out blocks in page size multiples. This could be improved, but at least allows for efficent DMA use.
    uint64_t block_step = PAGE_SIZE / block_size;
    off_t block_end = block_offset + block_count;
    off_t block;

    mutex_lock(&device->lock);
    for (block = block_offset; block < block_end;) {
        // This could only be non-zero on the first write, subsequent writes will be page aligned.
        uint64_t page_block_offset = block % block_step;
        uint64_t blocks_to_write = MIN(block - page_block_offset + block_step, (uint64_t) block_end) - block;

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
        free_phys_addr_mapping(mapped_page);

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

static uint64_t block_block_count(struct fs_device *device) {
    struct block_device *block_device = device->private;
    return block_device->block_count;
}

static uint32_t block_block_size(struct fs_device *device) {
    struct block_device *block_device = device->private;
    return block_device->block_size;
}

struct phys_page *block_generic_read_page(struct block_device *self, off_t block_offset) {
    struct phys_page *page = block_allocate_phys_page(self);
    if (!page) {
        return NULL;
    }
    page->block_offset = block_offset;

    uint64_t blocks_to_read = PAGE_SIZE / self->block_size;

    // NOTE: if this operation is ever blocking, we would need to allocate a permenant buffer.
    void *buffer = create_phys_addr_mapping(page->phys_addr);
    if (self->op->read(self, buffer, blocks_to_read, block_offset) != (int64_t) blocks_to_read) {
        free_phys_addr_mapping(buffer);
        drop_phys_page(page);
        return NULL;
    }
    free_phys_addr_mapping(buffer);

    return page;
}

int block_generic_sync_page(struct block_device *self, struct phys_page *page) {
    uint64_t blocks_to_write = PAGE_SIZE / self->block_size;

    // NOTE: if this operation is ever blocking, we would need to allocate a permenant buffer.
    void *buffer = create_phys_addr_mapping(page->phys_addr);
    if (self->op->write(self, buffer, blocks_to_write, page->block_offset) != (int64_t) blocks_to_write) {
        free_phys_addr_mapping(buffer);
        return -EIO;
    }
    free_phys_addr_mapping(buffer);

    return 0;
}

struct block_device *create_block_device(uint64_t block_count, uint32_t block_size, struct block_device_info info,
                                         struct block_device_ops *op, void *private_data) {
    struct block_device *block_device = malloc(sizeof(struct block_device));
    block_device->block_count = block_count;
    block_device->block_size = block_size;
    block_device->partition_offset = 0;
    block_device->partition_number = 0;
    block_device->block_hash_map = hash_create_hash_map(page_hash, page_equals, page_key);
    init_list(&block_device->lru_list);
    block_device->op = op;
    block_device->private_data = private_data;
    block_device->info = info;
    return block_device;
}

void block_register_device(struct block_device *block_device, const char *name, dev_t device_number) {
    struct fs_device *device = calloc(1, sizeof(struct fs_device));
    strcpy(device->name, name);
    device->device_number = device_number;
    device->mode = S_IFBLK | 0600;
    init_file_state(&device->file_state, true, true);
    init_mutex(&device->lock);
    device->ops = &block_device_ops;
    device->private = block_device;
    block_device->device = device;
    list_append(&block_devices, &block_device->list);
    dev_register(device);

    if (block_is_root_device(block_device)) {
        block_partition_device(block_device);
    }
}

struct phys_page *block_allocate_phys_page(struct block_device *block_device) {
    struct fs_device *device = block_device->device;

    // The device must not be locked when allocate_phys_page() is called, since in an effort to reclaim
    // physical memory, the page frame allocator may ask the device to trim its cache. This procedure must
    // aquire the device lock before doing so, and thus it cannot already be locked.
    mutex_unlock(&device->lock);
    struct phys_page *ret = allocate_phys_page();
    mutex_lock(&device->lock);

    return ret;
}

static void do_block_trim_cache(struct hash_entry *_device, void *closure __attribute__((unused))) {
    struct fs_device *device = hash_table_entry(_device, struct fs_device);
    if (!(device->mode & S_IFBLK)) {
        return;
    }

    // Since this is called by the page frame allocator, which is synchronized by a spin lock, mutex_lock()
    // cannot be used.
    if (!mutex_trylock(&device->lock)) {
        return;
    }

    struct block_device *block_device = device->private;
    for (size_t i = 0; i < 100 && !list_is_empty(&block_device->lru_list); i++) {
        block_shrink_cache(block_device);
    }
    mutex_unlock(&device->lock);
}

void block_trim_cache(void) {
    debug_log("Trimming block cache\n");
    hash_for_each(dev_device_hash_map(), do_block_trim_cache, NULL);
}

struct list_node *block_device_list(void) {
    return &block_devices;
}

int block_show_device(struct block_device *block_device, char *buffer, size_t _buffer_length) {
    int position = 0;
    int buffer_length = _buffer_length;
    position += snprintf(buffer + position, MAX(buffer_length - position, 0), "BLOCK_SIZE: %" PRIu32 "\n", block_device->block_size);
    position += snprintf(buffer + position, MAX(buffer_length - position, 0), "BLOCK_COUNT: %" PRIu64 "\n", block_device->block_count);
    position +=
        snprintf(buffer + position, MAX(buffer_length - position, 0), "TYPE: %s\n", block_device_type_to_string(block_device->info.type));

    char aux_buffer[128];
    block_show_device_id(block_device->info.disk_id, aux_buffer, sizeof(aux_buffer));
    position += snprintf(buffer + position, MAX(buffer_length - position, 0), "DISK_ID: %s\n", aux_buffer);
    block_show_device_id(block_device->info.partition_id, aux_buffer, sizeof(aux_buffer));
    position += snprintf(buffer + position, MAX(buffer_length - position, 0), "PARTITION_ID: %s\n", aux_buffer);
    block_show_device_id(block_device->info.filesystem_type_id, aux_buffer, sizeof(aux_buffer));
    position += snprintf(buffer + position, MAX(buffer_length - position, 0), "FS_TYPE_ID: %s\n", aux_buffer);
    block_show_device_id(block_device->info.filesystem_id, aux_buffer, sizeof(aux_buffer));
    position += snprintf(buffer + position, MAX(buffer_length - position, 0), "FS_ID: %s\n", aux_buffer);

    return position;
}

int block_show_device_id(struct block_device_id id, char *buffer, size_t buffer_length) {
    switch (id.type) {
        case BLOCK_ID_TYPE_MBR:
            return snprintf(buffer, buffer_length, "[MBR] %#X", id.mbr_id);
        case BLOCK_ID_TYPE_UUID: {
            char uuid_buffer[UUID_STRING_MAX];
            uuid_to_string(id.uuid, uuid_buffer, sizeof(uuid_buffer));
            return snprintf(buffer, buffer_length, "[UUID] %s", uuid_buffer);
        }
        default:
            return snprintf(buffer, buffer_length, "None");
    }
}

const char *block_device_type_to_string(enum block_device_type type) {
    switch (type) {
        case BLOCK_TYPE_DISK:
            return "Disk";
        case BLOCK_TYPE_PARTITION:
            return "Partition";
        default:
            return "Invalid";
    }
}
