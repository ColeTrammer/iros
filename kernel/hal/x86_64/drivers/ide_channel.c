#include <stdlib.h>
#include <sys/types.h>

#include <kernel/hal/x86_64/drivers/ide_channel.h>
#include <kernel/hal/x86_64/drivers/ide_controller.h>

enum ata_wait_result ata_wait_not_busy(struct ide_location location) {
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

void ata_select_drive_with_lba(struct ide_channel *channel, int drive, uint32_t lba) {
    uint8_t value = ATA_DRIVE_ALWAYS_1 | ATA_DRIVE_LBA_MODE | (drive << ATA_DRIVE_SELECT_BIT) | ((lba >> 24) & 0xF);
    outb(channel->location.io_base + ATA_DRIVE_OFFSET, value);
    if (channel->current_drive != drive) {
        io_wait_us(20);
        channel->current_drive = drive;
    }
}

struct ide_channel *ide_create_channel(struct ide_controller *controller, uint16_t io_base, uint16_t command_base,
                                       uint16_t ide_bus_master) {
    struct ide_channel *channel = calloc(1, sizeof(*channel));
    init_hw_device(&channel->hw_device, "IDE Channel", &controller->pci_device.hw_device, hw_device_id_none(), NULL, NULL);
    channel->hw_device.status = HW_STATUS_ACTIVE;

    channel->location.io_base = io_base;
    channel->location.command_base = command_base;
    channel->location.ide_bus_master = ide_bus_master;
    init_spinlock(&channel->lock);

    debug_log("Created IDE Channel: [ %#.4X, %#.4X, %#.4X ]\n", io_base, command_base, ide_bus_master);

    ata_write_device_control(channel->location, ATA_DEVICE_CONTROL_DISABLE_IRQS);

    // Ensure drive 0 will be selected
    channel->current_drive = 1;

    for (int drive = 0; drive < 2; drive++) {
        channel->drives[drive] = ata_detect_drive(channel, drive);
    }

    return channel;
}
