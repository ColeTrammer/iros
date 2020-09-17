#ifndef _KERNEL_HAL_GPT_H
#define _KERNEL_HAL_GPT_H 1

#include <stdint.h>

#include <kernel/util/uuid.h>

#define GPT_MBR_TYPE 0xEE

struct block_device;

struct gpt_header {
#define GPT_SIGNATURE 0x5452415020494645ULL
    uint64_t signature;
    uint32_t revisition;
    uint32_t header_size;
    uint32_t crc32;
    uint32_t reserved;
    uint64_t this_lba;
    uint64_t alt_lba;
    uint64_t first_usable_block;
    uint64_t last_usable_block;
    struct uuid disk_uuid;
    uint64_t partition_entry_start;
    uint32_t parittion_entry_count;
    uint32_t partition_entry_size;
    uint32_t partition_entry_crc32;
} __attribute__((packed));

struct gpt_partition_entry {
    struct uuid type_uuid;
    struct uuid partition_uuid;
    uint64_t start;
    uint64_t end;
    uint64_t attributes;
    uint16_t name[0];
} __attribute__((packed));

void gpt_partition_device(struct block_device *block_device);

#endif /* _KERNEL_HAL_GPT_H */
