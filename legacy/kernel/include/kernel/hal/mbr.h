#ifndef _KERNEL_HAL_MBR_H
#define _KERNEL_HAL_MBR_H 1

#include <stdint.h>

struct block_device;

struct mbr_partition {
    uint8_t drive_attributes;
    uint8_t chs_start[3];
#define MBR_EBR_CHS 0x05
#define MBR_EBR_LBA 0x0F
    uint8_t partition_type;
    uint8_t chs_last[3];
    uint32_t lba_start;
    uint32_t sector_count;
} __attribute__((packed));

struct mbr_table {
    uint8_t code1[218];
    uint16_t zero1;
    uint8_t original_physical_drive;
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t code2[216];
    uint32_t disk_signature;
    uint16_t zero2;
#define MBR_MAX_PARTITIONS 4
    struct mbr_partition partitions[MBR_MAX_PARTITIONS];
#define MBR_SIGNATURE 0xAA55
    uint16_t boot_signature;
};

_Static_assert(sizeof(struct mbr_table) == 512);

void mbr_partition_device(struct block_device *block_device);

#endif /* _KERNEL_HAL_MBR_H */
