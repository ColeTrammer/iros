#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>

#include <kernel/hal/x86/drivers/ata_drive.h>
#include <kernel/hal/x86/drivers/ide_channel.h>
#include <kernel/mem/page.h>
#include <kernel/mem/phys_page.h>
#include <kernel/mem/vm_allocator.h>

#define DMA_BUFFER_PAGES 1

static void ata_set_command(struct ide_location location, uint8_t command) {
    outb(location.io_base + ATA_COMMAND_OFFSET, command);
}

static void ata_set_command_and_delay(struct ide_location location, uint8_t command) {
    ata_set_command(location, command);
    for (size_t i = 0; i < 4; i++) {
        ata_read_alt_status(location);
    }
}

static void ata_setup_registers(struct ata_drive *drive, uint32_t offset, uint8_t sectors) {
    outb(drive->channel->location.io_base + ATA_FEATURES_OFFSET, 0);
    outb(drive->channel->location.io_base + ATA_SECTOR_COUNT_OFFSET, 0);
    outb(drive->channel->location.io_base + ATA_SECTOR_NUMBER_OFFSET, 0);
    outb(drive->channel->location.io_base + ATA_CYLINDER_LOW_OFFSET, 0);
    outb(drive->channel->location.io_base + ATA_CYLINDER_HIGH_OFFSET, 0);

    outb(drive->channel->location.io_base + ATA_SECTOR_COUNT_OFFSET, sectors);
    outb(drive->channel->location.io_base + ATA_SECTOR_NUMBER_OFFSET, offset & 0xFF);
    outb(drive->channel->location.io_base + ATA_CYLINDER_LOW_OFFSET, (offset >> 8) & 0xFF);
    outb(drive->channel->location.io_base + ATA_CYLINDER_HIGH_OFFSET, (offset >> 16) & 0xFF);
}

static int64_t ata_write_sectors_pio(struct block_device *self, const void *buffer, uint64_t n, off_t sector_offset) {
    struct ata_drive *drive = self->private_data;
    if (n == 0) {
        return 0;
    } else if (n > 255) {
        return -EINVAL;
    } else if (n == 255) {
        n = 0;
    }

    ide_channel_lock(drive->channel);

    ata_select_drive_with_lba(drive->channel, drive->drive, sector_offset);

    enum ata_wait_result wait_result = ata_wait_not_busy(drive->channel->location);
    if (wait_result != ATA_WAIT_RESULT_SUCCESS) {
        ide_channel_unlock(drive->channel);
        return -EIO;
    }

    ata_setup_registers(drive, sector_offset, n);

    uint64_t save = disable_interrupts_save();
    ata_set_command_and_delay(drive->channel->location, ATA_COMMAND_WRITE_PIO);

    const uint16_t *buf = (const uint16_t *) buffer;
    int64_t ret = n;
    for (uint64_t j = 0; j < n; j++) {
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
    ide_channel_unlock(drive->channel);
    return ret;
}

static int64_t ata_read_sectors_pio(struct block_device *self, void *buffer, uint64_t n, off_t sector_offset) {
    struct ata_drive *drive = self->private_data;
    if (n == 0) {
        return 0;
    } else if (n > 255) {
        return -EINVAL;
    } else if (n == 255) {
        n = 0;
    }

    assert(drive->channel);
    ide_channel_lock(drive->channel);

    ata_select_drive_with_lba(drive->channel, drive->drive, sector_offset);

    enum ata_wait_result wait_result = ata_wait_not_busy(drive->channel->location);
    if (wait_result != ATA_WAIT_RESULT_SUCCESS) {
        ide_channel_unlock(drive->channel);
        return -EIO;
    }

    ata_setup_registers(drive, sector_offset, n);

    uint64_t save = disable_interrupts_save();
    ata_set_command_and_delay(drive->channel->location, ATA_COMMAND_READ_PIO);

    uint16_t *buf = (uint16_t *) buffer;
    int64_t ret = n;

    for (uint64_t j = 0; j < n; j++) {
        wait_result = ata_wait_not_busy(drive->channel->location);
        if (wait_result != ATA_WAIT_RESULT_SUCCESS) {
            ret = j == 0 ? -EIO : (int64_t) j;
            goto done;
        }

        for (size_t i = 0; i < self->block_size / sizeof(uint16_t); i++) {
            buf[j * self->block_size / sizeof(uint16_t) + i] = inw(drive->channel->location.io_base + ATA_DATA_OFFSET);
        }
    }

done:
    interrupts_restore(save);
    ide_channel_unlock(drive->channel);
    return ret;
}

static int64_t ata_read_sectors_dma(struct block_device *self, void *buffer, uint64_t n, off_t sector_offset) {
    struct ata_drive *drive = self->private_data;
    size_t sectors = n;
    int ret = 1;
    if (n == 0) {
        return 0;
    } else if (n > DMA_BUFFER_PAGES * PAGE_SIZE / self->block_size) {
        return -EINVAL;
    }

    ide_channel_lock(drive->channel);

    drive->prdt[0].phys_addr = (uint32_t) get_phys_addr((uintptr_t) drive->dma_region->start);
    drive->prdt[0].size = (uint16_t) (self->block_size * sectors);
    ata_setup_prdt(drive);

    ata_select_drive_with_lba(drive->channel, drive->drive, sector_offset);

    enum ata_wait_result wait_result = ata_wait_not_busy(drive->channel->location);
    if (wait_result != ATA_WAIT_RESULT_SUCCESS) {
        goto done;
    }

    ata_setup_registers(drive, sector_offset, sectors);

    ata_set_command(drive->channel->location, ATA_COMMAND_READ_DMA);

    // Start bus master (Disable interrupts so that the irq doesn't come before we sleep)
    uint64_t save = disable_interrupts_save();
    outb(drive->channel->location.ide_bus_master, ATA_BUS_MASTER_COMMAND_START | ATA_BUS_MASTER_COMMAND_WRITE);
    ret = ata_wait_irq(drive->channel);
    interrupts_restore(save);

    if (ret) {
        goto done;
    }

    memcpy(buffer, (void *) drive->dma_region->start, self->block_size * n);

done:
    ide_channel_unlock(drive->channel);
    return ret ? -EIO : (int64_t) sectors;
}

static int64_t ata_write_sectors_dma(struct block_device *self, const void *buffer, uint64_t n, off_t sector_offset) {
    struct ata_drive *drive = self->private_data;
    size_t sectors = n;
    if (n == 0) {
        return 0;
    } else if (n > DMA_BUFFER_PAGES * PAGE_SIZE / self->block_size) {
        return -EINVAL;
    }

    memcpy((void *) drive->dma_region->start, buffer, n * self->block_size);

    ide_channel_lock(drive->channel);

    drive->prdt[0].phys_addr = (uint32_t) get_phys_addr(drive->dma_region->start);
    drive->prdt[0].size = (uint16_t) (self->block_size * sectors);
    ata_setup_prdt(drive);

    ata_select_drive_with_lba(drive->channel, drive->drive, sector_offset);

    enum ata_wait_result wait_result = ata_wait_not_busy(drive->channel->location);
    if (wait_result != ATA_WAIT_RESULT_SUCCESS) {
        ide_channel_unlock(drive->channel);
    }

    ata_setup_registers(drive, sector_offset, sectors);

    ata_set_command(drive->channel->location, ATA_COMMAND_WRITE_DMA);

    // Start bus master (Disable interrupts so that the irq doesn't come before we sleep)
    uint64_t save = disable_interrupts_save();
    outb(drive->channel->location.ide_bus_master + ATA_BUS_MASTER_COMMAND_OFFSET, ATA_BUS_MASTER_COMMAND_START);
    int ret = ata_wait_irq(drive->channel);
    interrupts_restore(save);

    return ret ? -EIO : (int64_t) sectors;
}

static struct phys_page *ata_read_page_dma(struct block_device *self, off_t sector_offset) {
    struct ata_drive *drive = self->private_data;
    uint64_t sectors = PAGE_SIZE / self->block_size;

    struct phys_page *page = block_allocate_phys_page(self);
    if (!page) {
        return NULL;
    }
    page->block_offset = sector_offset;

    ide_channel_lock(drive->channel);

    // Read directly into the phys_page
    drive->prdt[0].phys_addr = page->phys_addr;
    drive->prdt[0].size = PAGE_SIZE;
    ata_setup_prdt(drive);

    ata_select_drive_with_lba(drive->channel, drive->drive, sector_offset);

    enum ata_wait_result wait_result = ata_wait_not_busy(drive->channel->location);
    if (wait_result != ATA_WAIT_RESULT_SUCCESS) {
        drop_phys_page(page);
        page = NULL;
        goto done;
    }

    ata_setup_registers(drive, page->block_offset, sectors);

    ata_set_command(drive->channel->location, ATA_COMMAND_READ_DMA);

    // Start bus master (Disable interrupts so that the irq doesn't come before we sleep)
    uint64_t save = disable_interrupts_save();
    outb(drive->channel->location.ide_bus_master, ATA_BUS_MASTER_COMMAND_START | ATA_BUS_MASTER_COMMAND_WRITE);
    int ret = ata_wait_irq(drive->channel);
    interrupts_restore(save);

    if (ret) {
        drop_phys_page(page);
        page = NULL;
    }

done:
    ide_channel_unlock(drive->channel);
    return page;
}

static int ata_sync_page_dma(struct block_device *self, struct phys_page *page) {
    struct ata_drive *drive = self->private_data;
    uint64_t sectors = PAGE_SIZE / self->block_size;

    ide_channel_lock(drive->channel);

    // Write directly from the physical page.
    drive->prdt[0].phys_addr = page->phys_addr;
    drive->prdt[0].size = PAGE_SIZE;
    ata_setup_prdt(drive);

    ata_select_drive_with_lba(drive->channel, drive->drive, page->block_offset);

    enum ata_wait_result wait_result = ata_wait_not_busy(drive->channel->location);
    if (wait_result != ATA_WAIT_RESULT_SUCCESS) {
        ide_channel_unlock(drive->channel);
    }

    ata_setup_registers(drive, page->block_offset, sectors);

    ata_set_command(drive->channel->location, ATA_COMMAND_WRITE_DMA);

    // Start bus master (Disable interrupts so that the irq doesn't come before we sleep)
    uint64_t save = disable_interrupts_save();
    outb(drive->channel->location.ide_bus_master + ATA_BUS_MASTER_COMMAND_OFFSET, ATA_BUS_MASTER_COMMAND_START);
    int ret = ata_wait_irq(drive->channel);
    interrupts_restore(save);

    ide_channel_unlock(drive->channel);
    return ret;
}

static struct block_device_ops ata_pio_ops = {
    .read = ata_read_sectors_pio,
    .write = ata_write_sectors_pio,
    .read_page = block_generic_read_page,
    .sync_page = block_generic_sync_page,
};

static struct block_device_ops ata_dma_ops = {
    .read = ata_read_sectors_dma,
    .write = ata_write_sectors_dma,
    .read_page = ata_read_page_dma,
    .sync_page = ata_sync_page_dma,
};

static char drive_unique_index;

static struct ata_drive *ata_create_drive(struct ide_channel *channel, int drive_index, const char *model_name, uint64_t sector_count,
                                          bool supports_lba_48) {
    struct ata_drive *drive = calloc(1, sizeof(*drive));
    init_hw_device(&drive->hw_device, model_name, &channel->hw_device, hw_device_id_none(), NULL, NULL);
    drive->hw_device.status = HW_STATUS_ACTIVE;

    drive->channel = channel;
    drive->supports_lba_48 = supports_lba_48;
    drive->drive = drive_index;

    struct block_device_ops *ops = &ata_pio_ops;
    if (channel->can_bus_master) {
        drive->dma_region = vm_allocate_dma_region(PAGE_SIZE * DMA_BUFFER_PAGES);
        ops = &ata_dma_ops;
    }

    drive->block_device = create_block_device(sector_count, ATA_SECTOR_SIZE, block_device_info_none(BLOCK_TYPE_DISK), ops, drive);

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

    uint64_t sector_count;
    if (supports_lba_48) {
        sector_count = *(uint64_t *) (identify_buffer + ATA_IDENTIFY_MAX_LBA_48);
    } else {
        sector_count = *(uint32_t *) (identify_buffer + ATA_IDENTIFY_MAX_LBA);
    }

    debug_log("ATA Drive: [ %d, %s, %" PRIu64 ", %#.4X, %#.8X ]\n", drive, model_name, sector_count, capabilites, command_sets);
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

        debug_log("Unknown ATA drive type: [ %2X, %2X, %d ]\n", cl, ch, drive);
        return NULL;
    }

    status = ata_read_alt_status(channel->location);
    if (!(status & ATA_STATUS_DRQ)) {
        debug_log("ATA drive does not exist (status DRQ not set): [ %d ]\n", drive);
        return NULL;
    }

    return ata_process_identify_command(channel, drive);
}
