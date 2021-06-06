#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>

#include <kernel/hal/x86_64/drivers/ata_drive.h>
#include <kernel/hal/x86_64/drivers/ide_channel.h>

static void ata_set_command_and_delay(struct ide_location location, uint8_t command) {
    outb(location.io_base + ATA_COMMAND_OFFSET, command);
    for (size_t i = 0; i < 4; i++) {
        ata_read_alt_status(location);
    }
}

static void ata_setup_registers_pio(struct ata_drive *drive, uint32_t offset, uint8_t sectors) {
    ata_select_drive_with_lba(drive->channel, drive->drive, offset);

    outb(drive->channel->location.io_base + ATA_SECTOR_COUNT_OFFSET, sectors);
    outb(drive->channel->location.io_base + ATA_SECTOR_NUMBER_OFFSET, offset & 0xFF);
    outb(drive->channel->location.io_base + ATA_CYLINDER_LOW_OFFSET, (offset >> 8) & 0xFF);
    outb(drive->channel->location.io_base + ATA_CYLINDER_HIGH_OFFSET, (offset >> 16) & 0xFF);
}

static ssize_t ata_write_sectors_pio(struct block_device *self, const void *buffer, blkcnt_t n, off_t sector_offset) {
    struct ata_drive *drive = self->private_data;
    if (n == 0) {
        return 0;
    } else if (n > 255) {
        return -EINVAL;
    } else if (n == 255) {
        n = 0;
    }

    enum ata_wait_result wait_result = ata_wait_not_busy(drive->channel->location);
    if (wait_result != ATA_WAIT_RESULT_SUCCESS) {
        return -EIO;
    }

    ata_setup_registers_pio(drive, sector_offset, n);

    uint64_t save = disable_interrupts_save();
    ata_set_command_and_delay(drive->channel->location, ATA_COMMAND_WRITE);

    const uint16_t *buf = (const uint16_t *) buffer;
    ssize_t ret = n;
    for (blkcnt_t j = 0; j < n; j++) {
        wait_result = ata_wait_not_busy(drive->channel->location);
        if (wait_result != ATA_WAIT_RESULT_SUCCESS) {
            ret = -EIO;
            goto done;
        }

        for (size_t i = 0; i < self->block_size / sizeof(uint16_t); i++) {
            outw(drive->channel->location.io_base + ATA_DATA_OFFSET, buf[j * self->block_size / sizeof(uint16_t) + i]);
        }
    }

    // Clear cache after writing
    wait_result = ata_wait_not_busy(drive->channel->location);
    if (wait_result != ATA_WAIT_RESULT_SUCCESS) {
        ret = -EIO;
        goto done;
    }

    ata_set_command_and_delay(drive->channel->location, ATA_COMMAND_CACHE_FLUSH);

    wait_result = ata_wait_not_busy(drive->channel->location);
    if (wait_result != ATA_WAIT_RESULT_SUCCESS) {
        ret = -EIO;
        goto done;
    }

done:
    interrupts_restore(save);
    return ret;
}

static ssize_t ata_read_sectors_pio(struct block_device *self, void *buffer, blkcnt_t n, off_t sector_offset) {
    struct ata_drive *drive = self->private_data;
    if (n == 0) {
        return 0;
    } else if (n > 255) {
        return -EINVAL;
    } else if (n == 255) {
        n = 0;
    }

    enum ata_wait_result wait_result = ata_wait_not_busy(drive->channel->location);
    if (wait_result != ATA_WAIT_RESULT_SUCCESS) {
        return -EIO;
    }

    ata_setup_registers_pio(drive, sector_offset, n);

    uint64_t save = disable_interrupts_save();
    ata_set_command_and_delay(drive->channel->location, ATA_COMMAND_READ);

    uint16_t *buf = (uint16_t *) buffer;
    ssize_t ret = n;

    for (blkcnt_t j = 0; j < n; j++) {
        wait_result = ata_wait_not_busy(drive->channel->location);
        if (wait_result != ATA_WAIT_RESULT_SUCCESS) {
            ret = j == 0 ? -EIO : j;
            goto done;
        }

        for (size_t i = 0; i < self->block_size / sizeof(uint16_t); i++) {
            buf[j * self->block_size / sizeof(uint16_t) + i] = inw(drive->channel->location.io_base + ATA_DATA_OFFSET);
        }
    }

done:
    interrupts_restore(save);
    return ret;
}

static struct block_device_ops ata_pio_ops = {
    .read = ata_read_sectors_pio,
    .write = ata_write_sectors_pio,
    .read_page = block_generic_read_page,
    .sync_page = block_generic_sync_page,
};

static char drive_unique_index;

static struct ata_drive *ata_create_drive(struct ide_channel *channel, int drive_index, const char *model_name, blkcnt_t sector_count,
                                          bool supports_lba_48) {
    struct ata_drive *drive = calloc(1, sizeof(*drive));
    init_hw_device(&drive->hw_device, model_name, &channel->hw_device, hw_device_id_none(), NULL, NULL);
    drive->hw_device.status = HW_STATUS_ACTIVE;

    drive->channel = channel;
    drive->supports_lba_48 = supports_lba_48;
    drive->drive = drive_index;

    drive->block_device = create_block_device(sector_count, ATA_SECTOR_SIZE, block_device_info_none(BLOCK_TYPE_DISK), &ata_pio_ops, drive);

    char name[16];
    snprintf(name, sizeof(name) - 1, "sd%c", 'a' + drive_unique_index);
    block_register_device(drive->block_device, name, 0x00500 + 16 * drive_unique_index);

    drive_unique_index++;
    return drive;
}

static struct ata_drive *ata_process_identify_command(struct ide_channel *channel, int drive) {
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
        return NULL;
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
    return ata_create_drive(channel, drive, model_name, sector_count, supports_lba_48);
}

struct ata_drive *ata_detect_drive(struct ide_channel *channel, int drive) {
    ata_do_select_drive(channel->location, drive);

    outb(channel->location.io_base + ATA_SECTOR_COUNT_OFFSET, 0);
    outb(channel->location.io_base + ATA_SECTOR_NUMBER_OFFSET, 0);
    outb(channel->location.io_base + ATA_CYLINDER_LOW_OFFSET, 0);
    outb(channel->location.io_base + ATA_CYLINDER_HIGH_OFFSET, 0);
    outb(channel->location.io_base + ATA_COMMAND_OFFSET, ATA_COMMAND_INDENTIFY);

    uint8_t status = ata_read_alt_status(channel->location);
    if (!status) {
        debug_log("ATA drive does not exist (status=0): [ %d ]\n", drive);
        return NULL;
    }

    enum ata_wait_result wait_result = ata_wait_not_busy(channel->location);
    if (wait_result == ATA_WAIT_RESULT_TIMEOUT) {
        debug_log("ATA drive failed to respond to IDENTIFY request: [ %d ]\n", drive);
        return NULL;
    }

    if (wait_result == ATA_WAIT_RESULT_ERROR) {
        // Check for ATAPI or SATA drives
        uint8_t cl = inb(channel->location.io_base + ATA_CYLINDER_LOW_OFFSET);
        uint8_t ch = inb(channel->location.io_base + ATA_CYLINDER_HIGH_OFFSET);

        if (cl == 0x14 && ch == 0xEB) {
            debug_log("ATAPI drive detected, but is unsupported: [ %d ]\n", drive);
            return NULL;
        }

        if (cl == 0x3C && ch == 0x3C) {
            debug_log("SATA drive detected, but is unsupported: [ %d ]\n", drive);
            return NULL;
        }
    }

    status = ata_read_alt_status(channel->location);
    if (!(status & ATA_STATUS_DRQ)) {
        debug_log("ATA drive does not exist (status DRQ not set): [ %d ]\n", drive);
        return NULL;
    }

    return ata_process_identify_command(channel, drive);
}
