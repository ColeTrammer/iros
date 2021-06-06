#ifndef _KERNEL_HAL_X86_64_DRIVERS_ATA_PIO_H
#define _KERNEL_HAL_X86_64_DRIVERS_ATA_PIO_H 1

#include <kernel/hal/hw_device.h>
#include <kernel/proc/wait_queue.h>

#define ATA1_IRQ 14
#define ATA2_IRQ 15
#define ATA3_IRQ 14
#define ATA4_IRQ 15

#define ATA_SECTOR_SIZE 512

#define ATA_IDENTIFY_MODEL_NAME_SIZE  40
#define ATA_IDENTIFY_MODEL_NAME_START 54
#define ATA_IDENTIFY_CAPABILITIES     98
#define ATA_IDENTIFY_MAX_LBA          120
#define ATA_IDENTIFY_COMMAND_SETS     164
#define ATA_IDENTIFY_MAX_LBA_48       200

#define ATA_CAPABILITY_LBA (1 << 9)

#define ATA_COMMAND_SET_LBA_48 (1 << 26)

#define ATA_DATA_OFFSET          0
#define ATA_ERROR_OFFSET         1
#define ATA_FEATURES_OFFSET      1
#define ATA_SECTOR_COUNT_OFFSET  2
#define ATA_SECTOR_NUMBER_OFFSET 3
#define ATA_CYLINDER_LOW_OFFSET  4
#define ATA_CYLINDER_HIGH_OFFSET 5
#define ATA_DRIVE_OFFSET         6
#define ATA_STATUS_OFFSET        7
#define ATA_COMMAND_OFFSET       7

#define ATA_ALT_STATUS_OFFSET     0
#define ATA_DEVICE_CONTROL_OFFSET 0
#define ATA_DRIVE_ADDRESS_OFFSET  1

#define ATA_BUS_MASTER_COMMAND_OFFSET 0
#define ATA_BUS_MASTER_STATUS_OFFSET  2
#define ATA_BUS_MASTER_PRD_OFFSET     4

#define ATA_BUS_MASTER_COMMAND_START (1 << 0)
#define ATA_BUS_MASTER_COMMAND_WRITE (1 << 3)

#define ATA_BUS_MASTER_STATUS_DID_IRQ (1 << 2)
#define ATA_BUS_MASTER_STATUS_ERROR   (1 << 1)

#define ATA_DEVICE_CONTROL_DISABLE_IRQS (1 << 1)
#define ATA_DEVICE_CONTROL_RESET_BIT    (1 << 2)

#define ATA_DRIVE_SELECT_BIT 4
#define ATA_DRIVE_ALWAYS_1   0xA0
#define ATA_DRIVE_LBA_MODE   (1 << 6)

#define ATA_ERROR_AMNF  (1 << 0)
#define ATA_ERROR_TKZNF (1 << 1)
#define ATA_ERROR_ABRT  (1 << 2)
#define ATA_ERROR_MCR   (1 << 3)
#define ATA_ERROR_IDNF  (1 << 4)
#define ATA_ERROR_MC    (1 << 5)
#define ATA_ERROR_UNC   (1 << 6)
#define ATA_ERROR_BBK   (1 << 7)

#define ATA_STATUS_ERR  (1 << 0)
#define ATA_STATUS_IDX  (1 << 1)
#define ATA_STATUS_CORR (1 << 2)
#define ATA_STATUS_DRQ  (1 << 3)
#define ATA_STATUS_SRV  (1 << 4)
#define ATA_STATUS_DF   (1 << 5)
#define ATA_STATUS_RDY  (1 << 6)
#define ATA_STATUS_BSY  (1 << 7)

#define ATA_COMMAND_READ        0x20
#define ATA_COMMAND_READ_DMA    0x25
#define ATA_COMMAND_WRITE       0x30
#define ATA_COMMAND_WRITE_DMA   0x35
#define ATA_COMMAND_CACHE_FLUSH 0xE7
#define ATA_COMMAND_INDENTIFY   0xEC

#define ATA_PRD_END 0x8000U

struct ata_physical_range_descriptor {
    uint32_t phys_addr;
    uint16_t size;
    uint16_t flag;
} __attribute__((packed));

#endif /* _KERNEL_HAL_X86_64_DRIVERS_ATA_PIO_H */
