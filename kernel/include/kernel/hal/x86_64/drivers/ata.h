#ifndef _KERNEL_HAL_X86_64_DRIVERS_ATA_PIO_H
#define _KERNEL_HAL_X86_64_DRIVERS_ATA_PIO_H 1

#define ATA1_IO_BASE 0x1F0
#define ATA1_CONTROL_BASE 0x3F6

#define ATA2_IO_BASE 0x170
#define ATA2_CONTROL_BASE 0x376

#define ATA3_IO_BASE 0x1E8
#define ATA3_CONTROL_BASE 0x3E6

#define ATA4_IO_BASE 0x168
#define ATA4_CONTROL_BASE 0x366

#define ATA_SECTOR_SIZE 512

#define ATA_DATA_OFFSET 0
#define ATA_ERROR_OFFSET 1
#define ATA_FEATURES_OFFSET 1
#define ATA_SECTOR_COUNT_OFFSET 2
#define ATA_SECTOR_NUMBER_OFFSET 3
#define ATA_CYLINDER_LOW_OFFSET 4
#define ATA_CYLINDER_HIGH_OFFSET 5
#define ATA_DRIVE_OFFSET 6
#define ATA_STATUS_OFFSET 7
#define ATA_COMMAND_OFFSET 7

#define ATA_ALT_STATUS_OFFSET 0
#define ATA_DEVICE_OFFSET 0
#define ATA_DRIVE_ADDRESS_OFFSET 1

#define ATA_ERROR_AMNF (1 << 0)
#define ATA_ERROR_TKZNF (1 << 1)
#define ATA_ERROR_ABRT (1 << 2)
#define ATA_ERROR_MCR (1 << 3)
#define ATA_ERROR_IDNF (1 << 4)
#define ATA_ERROR_MC (1 << 5)
#define ATA_ERROR_UNC (1 << 6)
#define ATA_ERROR_BBK (1 << 7)

#define ATA_STATUS_ERR (1 << 0)
#define ATA_STATUS_IDX (1 << 1)
#define ATA_STATUS_CORR (1 << 2)
#define ATA_STATUS_DRQ (1 << 3)
#define ATA_STATUS_SRV (1 << 4)
#define ATA_STATUS_DF (1 << 5)
#define ATA_STATUS_RDY (1 << 6)
#define ATA_STATUS_BSY (1 << 7)

#define ATA_COMMAND_READ 0x20
#define ATA_COMMAND_WRITE 0x30
#define ATA_COMMAND_CACHE_FLUSH 0xE7
#define ATA_COMMAND_INDENTIFY 0xEC

struct ata_port_info {
    uint16_t io_base;
    uint16_t control_base;
    bool is_slave;
};

struct ata_device_data {
    struct ata_port_info *port_info;
    size_t sector_size;
    size_t num_sectors;
};

void init_ata();

#endif /* _KERNEL_HAL_X86_64_DRIVERS_ATA_PIO_H */