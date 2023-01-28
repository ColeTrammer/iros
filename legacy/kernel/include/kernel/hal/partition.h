#ifndef _KERNEL_HAL_PARTITION_H
#define _KERNEL_HAL_PARTITION_H 1

#include <sys/types.h>

struct block_device_info;
struct block_device;

void block_partition_device(struct block_device *block_device);
struct block_device *create_and_register_partition_device(struct block_device *root_device, uint64_t block_count, off_t partition_offset,
                                                          int partition_number, struct block_device_info info);

#endif /* _KERNEL_HAL_PARTITION_H */
