#ifndef _KERNEL_HAL_X86_64_DRIVERS_IDE_CONTROLLER_H
#define _KERNEL_HAL_X86_64_DRIVERS_IDE_CONTROLLER_H 1

#include <kernel/hal/pci.h>
#include <kernel/util/spinlock.h>

#define IDE_CONTROLLER_IF_PRIMARY_MODE_SELECT         (1 << 0)
#define IDE_CONTROLLER_IF_PRIMARY_MODE_CAPABILITIES   (1 << 1)
#define IDE_CONTROLLER_IF_SECONDARY_MODE_SELECT       (1 << 2)
#define IDE_CONTROLLER_IF_SECONDARY_MODE_CAPABILITIES (1 << 3)
#define IDE_CONTROLLER_IF_BUS_MASTER_SUPPORTED        (1 << 7)

#define IDE_CONTROLLER_PCI_CLASS    0x01
#define IDE_CONTROLLER_PCI_SUBCLASS 0x01

#define IDE_CONTROLLER_CHANNEL0_DEFAULT_IO_BASE      0x1F0
#define IDE_CONTROLLER_CHANNEL0_DEFAULT_COMMAND_BASE 0x3F6
#define IDE_CONTROLLER_CHANNEL1_DEFAULT_IO_BASE      0x170
#define IDE_CONTROLLER_CHANNEL1_DEFAULT_COMMAND_BASE 0x376

struct ide_location {
    uint16_t io_base;
    uint16_t command_base;
    uint16_t ide_bus_master;
};

struct ide_channel {
    struct hw_device hw_device;
    struct ide_location location;
    spinlock_t lock;
};

struct ide_controller {
    struct pci_device pci_device;
    struct ide_channel *channels[2];
};

#endif /* _KERNEL_HAL_X86_64_DRIVERS_IDE_CONTROLLER_H */
