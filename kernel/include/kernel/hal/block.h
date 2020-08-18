#ifndef _KERNEL_HAL_BLOCK_H
#define _KERNEL_HAL_BLOCK_H 1

#include <sys/types.h>

struct block_device;
struct phys_page;

struct block_device_ops {
    blkcnt_t (*read)(struct block_device *self, void *buf, blkcnt_t block_count, off_t block_offset);
    blkcnt_t (*write)(struct block_device *self, const void *buf, blkcnt_t block_count, off_t block_offset);
};

struct block_device {
    blkcnt_t block_count;
    blksize_t block_size;
    struct block_device_ops *op;
    void *private_data;
};

struct block_device *create_block_device(blkcnt_t block_count, blksize_t block_size, struct block_device_ops *op, void *private_data);
void block_register_device(struct block_device *block_device, dev_t device_number);

#endif /* _KERNEL_HAL_BLOCK_H */
