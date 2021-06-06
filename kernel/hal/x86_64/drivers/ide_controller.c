#include <stdlib.h>
#include <sys/types.h>

#include <kernel/arch/x86_64/asm_utils.h>
#include <kernel/hal/output.h>
#include <kernel/hal/pci_driver.h>
#include <kernel/hal/x86_64/drivers/ata.h>
#include <kernel/hal/x86_64/drivers/ide_controller.h>

static void ata_write_device_control(struct ide_location location, uint8_t value) {
    outb(location.command_base + ATA_DEVICE_CONTROL_OFFSET, value);
}

static uint8_t ata_read_alt_status(struct ide_location location) {
    return inb(location.command_base + ATA_ALT_STATUS_OFFSET);
}

static void ata_do_select_drive(struct ide_location location, bool drive) {
    outb(location.io_base + ATA_DRIVE_OFFSET, ATA_DRIVE_ALWAYS_1 | (drive << ATA_DRIVE_SELECT_BIT));
    io_wait_us(20);
}

enum ata_wait_result {
    ATA_WAIT_RESULT_SUCCESS,
    ATA_WAIT_RESULT_ERROR,
    ATA_WAIT_RESULT_TIMEOUT,
};

static enum ata_wait_result ata_wait_not_busy(struct ide_location location) {
    for (int i = 0; i < 2000; i++) {
        uint8_t status = ata_read_alt_status(location);
        if (status & ATA_STATUS_ERR) {
            return ATA_WAIT_RESULT_ERROR;
        }

        if (!(status & ATA_STATUS_BSY)) {
            return ATA_WAIT_RESULT_SUCCESS;
        }
    }
    return ATA_WAIT_RESULT_TIMEOUT;
}

static struct ide_channel *ide_create_channel(struct ide_controller *controller, uint16_t io_base, uint16_t command_base,
                                              uint16_t ide_bus_master) {
    struct ide_channel *channel = calloc(1, sizeof(*channel));
    init_hw_device(&channel->hw_device, "IDE Channel", &controller->pci_device.hw_device, hw_device_id_none(), NULL, NULL);

    channel->location.io_base = io_base;
    channel->location.command_base = command_base;
    channel->location.ide_bus_master = ide_bus_master;
    init_spinlock(&channel->lock);

    debug_log("Created IDE Channel: [ %#.4X, %#.4X, %#.4X ]\n", io_base, command_base, ide_bus_master);

    ata_write_device_control(channel->location, ATA_DEVICE_CONTROL_DISABLE_IRQS);

    for (int drive = 0; drive < 2; drive++) {
        ata_do_select_drive(channel->location, drive);

        outb(channel->location.io_base + ATA_SECTOR_COUNT_OFFSET, 0);
        outb(channel->location.io_base + ATA_SECTOR_NUMBER_OFFSET, 0);
        outb(channel->location.io_base + ATA_CYLINDER_LOW_OFFSET, 0);
        outb(channel->location.io_base + ATA_CYLINDER_HIGH_OFFSET, 0);
        outb(channel->location.io_base + ATA_COMMAND_OFFSET, ATA_COMMAND_INDENTIFY);

        uint8_t status = ata_read_alt_status(channel->location);
        if (!status) {
            debug_log("ATA drive does not exist (status=0): [ %d ]\n", drive);
            continue;
        }

        enum ata_wait_result wait_result = ata_wait_not_busy(channel->location);
        if (wait_result == ATA_WAIT_RESULT_TIMEOUT) {
            debug_log("ATA drive failed to respond to IDENTIFY request: [ %d ]\n", drive);
            continue;
        }

        if (wait_result == ATA_WAIT_RESULT_ERROR) {
            // Check for ATAPI or SATA drives
            uint8_t cl = inb(channel->location.io_base + ATA_CYLINDER_LOW_OFFSET);
            uint8_t ch = inb(channel->location.io_base + ATA_CYLINDER_HIGH_OFFSET);

            if (cl == 0x14 && ch == 0xEB) {
                debug_log("ATAPI drive detected, but is unsupported: [ %d ]\n", drive);
                continue;
            }

            if (cl == 0x3C && ch == 0x3C) {
                debug_log("SATA drive detected, but is unsupported: [ %d ]\n", drive);
                continue;
            }
        }

        status = ata_read_alt_status(channel->location);
        if (!(status & ATA_STATUS_DRQ)) {
            debug_log("ATA drive does not exist (status DRQ not set): [ %d ]\n", drive);
            continue;
        }

        uint8_t identify_buffer[ATA_SECTOR_SIZE] = { 0 };
        for (size_t i = 0; i < ATA_SECTOR_SIZE / sizeof(uint16_t); i++) {
            ((uint16_t *) identify_buffer)[i] = inw(channel->location.io_base + ATA_DATA_OFFSET);
        }

        // Clobber memory because GCC thinks identify_buffer is not written to (because the cast is too confusing apparently).
        // Otherwise, GCC will "optimize" later accesses by setting them to 0 incorrectly.
        asm volatile("" ::: "memory");

        // The model name string is reported in the wrong byte order, so it must be reversed.
        char model_name[ATA_IDENTIFY_MODEL_NAME_SIZE + 1] = { 0 };
        for (size_t i = 0; i < ATA_IDENTIFY_MODEL_NAME_SIZE; i += 2) {
            model_name[i] = ((char *) identify_buffer)[ATA_IDENTIFY_MODEL_NAME_START + i + 1];
            model_name[i + 1] = ((char *) identify_buffer)[ATA_IDENTIFY_MODEL_NAME_START + i];
        }

        // Remove trailing spaces
        for (int i = ATA_IDENTIFY_MODEL_NAME_SIZE - 1; i >= 0; i--) {
            if (model_name[i] == ' ') {
                model_name[i] = '\0';
                continue;
            }
            break;
        }

        uint16_t capabilites = *(uint16_t *) (identify_buffer + ATA_IDENTIFY_CAPABILITIES);
        if (!(capabilites & ATA_CAPABILITY_LBA)) {
            debug_log("ATA Drive does not support LBA, so won't be used: [ %d, %s ]\n", drive, model_name);
            continue;
        }

        uint32_t command_sets = *(uint32_t *) (identify_buffer + ATA_IDENTIFY_COMMAND_SETS);
        bool supports_lba_48 = !!(command_sets & ATA_COMMAND_SET_LBA_48);

        blkcnt_t sector_count;
        if (supports_lba_48) {
            sector_count = *(uint64_t *) (identify_buffer + ATA_IDENTIFY_MAX_LBA_48);
        } else {
            sector_count = *(uint32_t *) (identify_buffer + ATA_IDENTIFY_MAX_LBA);
        }

        debug_log("ATA Drive: [ %d, %s, %lu, %#.4X, %#.8X ]\n", drive, model_name, sector_count, capabilites, command_sets);
    }

    return channel;
}

static uint16_t ide_get_io_base(struct pci_device_location location, uint8_t bar, uint16_t default_value) {
    uint32_t value = pci_config_read32(location, PCI_CONFIG_BAR(bar)) & ~1;
    if (!value) {
        return default_value;
    }
    return value;
}

static struct pci_device *ide_create(struct hw_device *parent, struct pci_device_location location, struct pci_device_id id,
                                     struct pci_device_info info) {
    struct ide_controller *controller = calloc(1, sizeof(*controller));
    init_pci_device(&controller->pci_device, location, info);
    init_hw_device(&controller->pci_device.hw_device, "IDE Controller", parent, hw_device_id_pci(id), NULL, NULL);

    uint32_t channel0_io_base = ide_get_io_base(location, 0, IDE_CONTROLLER_CHANNEL0_DEFAULT_IO_BASE);
    uint32_t channel0_command_base = ide_get_io_base(location, 1, IDE_CONTROLLER_CHANNEL0_DEFAULT_COMMAND_BASE);
    uint32_t channel1_io_base = ide_get_io_base(location, 2, IDE_CONTROLLER_CHANNEL1_DEFAULT_IO_BASE);
    uint32_t channel1_command_base = ide_get_io_base(location, 3, IDE_CONTROLLER_CHANNEL1_DEFAULT_COMMAND_BASE);
    uint32_t ide_bus_master_base = ide_get_io_base(location, 4, 0);

    debug_log("Detected IDE Controller: [ %#.2X, %#.4X, %#.4X, %#.4X, %#.4X, %#.4X ]\n", info.programming_interface, channel0_io_base,
              channel0_command_base, channel1_io_base, channel1_command_base, ide_bus_master_base);

    controller->channels[0] = ide_create_channel(controller, channel0_io_base, channel0_command_base, ide_bus_master_base);
    controller->channels[1] = ide_create_channel(controller, channel1_io_base, channel1_command_base, ide_bus_master_base + 8);

    return &controller->pci_device;
}

static struct pci_driver_ops ide_ops = {
    .create = ide_create,
};

static struct pci_device_info ide_device_info_table[] = {
    { .class_code = IDE_CONTROLLER_PCI_CLASS, .subclass_code = IDE_CONTROLLER_PCI_SUBCLASS },
};

static struct pci_driver ide_driver = {
    .name = "IDE Controller Driver",
    .device_info_table = ide_device_info_table,
    .device_info_count = sizeof(ide_device_info_table) / sizeof(ide_device_info_table[0]),
    .ops = &ide_ops,
};

PCI_DRIVER_INIT(ide_driver);
