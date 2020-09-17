#ifndef _KERNEL_HAL_BLOCK_H
#define _KERNEL_HAL_BLOCK_H 1

#include <sys/types.h>

#include <kernel/util/list.h>

struct block_device;
struct device;
struct hash_map;
struct phys_page;

struct block_device_ops {
    blkcnt_t (*read)(struct block_device *self, void *buf, blkcnt_t block_count, off_t block_offset);
    blkcnt_t (*write)(struct block_device *self, const void *buf, blkcnt_t block_count, off_t block_offset);
    struct phys_page *(*read_page)(struct block_device *self, off_t block_offset);
    int (*sync_page)(struct block_device *self, struct phys_page *page);
};

struct block_device {
    struct device *device;
    blkcnt_t block_count;
    blksize_t block_size;
    off_t partition_offset;
    int partition_number;
    struct hash_map *block_hash_map;
    struct list_node lru_list;
    struct block_device_ops *op;
    void *private_data;
};

struct phys_page *block_generic_read_page(struct block_device *self, off_t block_offset);
int block_generic_sync_page(struct block_device *self, struct phys_page *page);

void block_trim_cache(void);
struct phys_page *block_allocate_phys_page(struct block_device *block_device);
struct block_device *create_block_device(blkcnt_t block_count, blksize_t block_size, struct block_device_ops *op, void *private_data);
void block_register_device(struct block_device *block_device, dev_t device_number);

static inline bool block_is_root_device(struct block_device *block_device) {
    return block_device->partition_number == 0;
}

#endif /* _KERNEL_HAL_BLOCK_H */
