#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <kernel/fs/dev.h>
#include <kernel/hal/hal.h>
#include <kernel/hal/output.h>
#include <kernel/hal/x86_64/drivers/ata.h>
#include <kernel/hal/x86_64/drivers/serial.h>
#include <kernel/arch/x86_64/asm_utils.h>

static void ata_wait(struct ata_port_info *info) {
    for (size_t i = 0; i < 4; i++) {
        inb(info->control_base + ATA_ALT_STATUS_OFFSET);
    }
}

static void ata_wait_not_busy(struct ata_port_info *info) {
    while (inb(info->io_base + ATA_STATUS_OFFSET) & ATA_STATUS_BSY);
}

static void ata_wait_ready(struct ata_port_info *info) {
    while (!(inb(info->io_base + ATA_STATUS_OFFSET) & ATA_STATUS_RDY));
}

static uint8_t ata_read_status(struct ata_port_info *info) {
    return inb(info->control_base + ATA_ALT_STATUS_OFFSET);
}

static void ata_select_device(struct ata_port_info *info) {
    outb(info->io_base + ATA_DRIVE_OFFSET, 0xA0 | (info->is_slave << 4));
}

static uint8_t ata_get_error(struct ata_port_info *info) {
    if (inb(info->control_base + ATA_ALT_STATUS_OFFSET) & ATA_STATUS_ERR) {
        return inb(info->io_base + ATA_ERROR_OFFSET);
    }

    return 0;
}

static void ata_set_lda_offset_and_device(struct ata_port_info *info, uint32_t offset) {
    outb(info->io_base + ATA_DRIVE_OFFSET, 0xE0 | (info->is_slave << 4) | ((offset >> 24) & 0xF));
    outb(info->io_base + ATA_SECTOR_NUMBER_OFFSET, offset & 0xFF);
    outb(info->io_base + ATA_CYLINDER_LOW_OFFSET, (offset >> 8) & 0xFF);
    outb(info->io_base + ATA_CYLINDER_HIGH_OFFSET, (offset >> 16) & 0xFF);
}

static void ata_set_sector_count(struct ata_port_info *info, uint8_t n) {
    outb(info->io_base + ATA_SECTOR_COUNT_OFFSET, n);
}

static void ata_set_command(struct ata_port_info *info, uint8_t command) {
    outb(info->io_base + ATA_COMMAND_OFFSET, command);
}

static uint16_t ata_read_word(struct ata_port_info *info) {
    return inw(info->io_base + ATA_DATA_OFFSET);
}

static void ata_write_word(struct ata_port_info *info, uint16_t val) {
    outw(info->io_base + ATA_DATA_OFFSET, val);
}

static bool ata_indentify(struct ata_port_info *info, uint16_t *buf) {
    ata_select_device(info);

    outb(info->io_base + ATA_SECTOR_COUNT_OFFSET, 0);
    outb(info->io_base + ATA_SECTOR_NUMBER_OFFSET, 0);
    outb(info->io_base + ATA_CYLINDER_LOW_OFFSET, 0);
    outb(info->io_base + ATA_CYLINDER_HIGH_OFFSET, 0);

    ata_set_command(info, ATA_COMMAND_INDENTIFY);

    /* Drive does not exist */
    if (ata_read_status(info) == 0) {
        return false;
    }

    ata_wait_not_busy(info);

    uint8_t status = ata_read_status(info);
    while (!(status & ATA_STATUS_DRQ)) {
        /* Give up if the drive errors */
        if (status & ATA_STATUS_ERR) {
            return false;
        }

        status = ata_read_status(info);
    }

    /* Read result into buffer */
    for (size_t i = 0; i < 256; i++) {
        buf[i] = ata_read_word(info);
    }

    return true;
}

static ssize_t ata_read_sectors(struct ata_device_data *data, size_t offset, void *buffer, size_t n) {
    if (n == 0) {
        return 0;
    } else if (n > 255) {
        return -EINVAL;
    } else if (n == 255) {
        n = 0;
    }

    assert(data->sector_size == 512);
    debug_log("Ata read sectors: [ %#.16lX, %#.16lX ]\n", offset, n);

    uint64_t flags = disable_interrupts_save();

    ata_wait_not_busy(data->port_info);

    ata_set_sector_count(data->port_info, (uint8_t) n);

    ata_set_lda_offset_and_device(data->port_info, offset);

    ata_wait_ready(data->port_info);
    ata_set_command(data->port_info, ATA_COMMAND_READ);

    uint16_t *buf = (uint16_t*) buffer;

    for (size_t j = 0; j < n; j++) {
        ata_wait(data->port_info);
        ata_wait_not_busy(data->port_info);

        if (ata_read_status(data->port_info) & ATA_STATUS_ERR || !(ata_read_status(data->port_info) & ATA_STATUS_DRQ)) {
            if (flags & INTERRUPS_ENABLED_FLAG) {
                enable_interrupts();
            }

            return -EIO;
        }

        for (size_t i = 0; i < data->sector_size / sizeof(uint16_t); i++) {
            buf[j * data->sector_size / sizeof(uint16_t) + i] = ata_read_word(data->port_info);
        }
    }

    if (flags & INTERRUPS_ENABLED_FLAG) {
        enable_interrupts();
    }

    return n * data->sector_size;
}

static ssize_t ata_write_sectors(struct ata_device_data *data, size_t offset, const void *buffer, size_t n) {
    if (n == 0) {
        return 0;
    } else if (n > 255) {
        return -EINVAL;
    } else if (n == 255) {
        n = 0;
    }

    uint64_t flags = disable_interrupts_save();

    ata_wait_not_busy(data->port_info);

    ata_set_sector_count(data->port_info, (uint8_t) n);

    ata_set_lda_offset_and_device(data->port_info, offset);

    ata_wait_ready(data->port_info);
    ata_set_command(data->port_info, ATA_COMMAND_WRITE);

    const uint16_t *buf = (const uint16_t*) buffer;

    for (size_t j = 0; j < n; j++) {
        ata_wait(data->port_info);
        ata_wait_not_busy(data->port_info);

        if (ata_get_error(data->port_info)) {
            if (flags & INTERRUPS_ENABLED_FLAG) {
                enable_interrupts();
            }

            /* Might have to also reset the drive */
            return -EIO;
        }

        for (size_t i = 0; i < data->sector_size / sizeof(uint16_t); i++) {
            ata_write_word(data->port_info, buf[j * data->sector_size + i]);
        }
    }

    /* Clear cache after writing */
    ata_wait_not_busy(data->port_info);
    ata_set_command(data->port_info, ATA_COMMAND_CACHE_FLUSH);
    ata_wait_not_busy(data->port_info);

    if (ata_get_error(data->port_info)) {
        if (flags & INTERRUPS_ENABLED_FLAG) {
            enable_interrupts();
        }

        /* Might have to also reset the drive */
        return -EIO;
    }

    if (flags & INTERRUPS_ENABLED_FLAG) {
        enable_interrupts();
    }

    return n * data->sector_size;
}

static bool ata_device_exists(struct ata_port_info *info, uint16_t *buf) {
    if (ata_read_status(info) == 0xFF) {
        return false;
    }

    ata_wait_not_busy(info);
    ata_select_device(info);
    ata_wait(info);

    uint8_t cl = inb(info->io_base + ATA_CYLINDER_LOW_OFFSET);
    uint8_t ch = inb(info->io_base + ATA_CYLINDER_HIGH_OFFSET);

    /* Otherwise it is a different device */
    if (cl != 0 || ch != 0) {
        return false;
    }

    if (!ata_indentify(info, buf)) {
        return false;
    }

    return true;
}

static ssize_t ata_read(struct device *device, struct file *file, void *buffer, size_t n) {
    debug_log("Ata read: [ %#.16lX, %#.16lX ]\n", file->position, n);

    if (n % ((struct ata_device_data*) device->private)->sector_size == 0 && file->position % ((struct ata_device_data*) device->private)->sector_size == 0) {
        return ata_read_sectors(device->private, file->position / ((struct ata_device_data*) device->private)->sector_size, buffer, n / ((struct ata_device_data*) device->private)->sector_size);
    }

    return -EINVAL;
}

static ssize_t ata_write(struct device *device, struct file *file, const void *buffer, size_t n) {
    if (n % ((struct ata_device_data*) device->private)->sector_size == 0 && file->position % ((struct ata_device_data*) device->private)->sector_size == 0) {
        return ata_write_sectors(device->private, file->position / ((struct ata_device_data*) device->private)->sector_size, buffer, n / ((struct ata_device_data*) device->private)->sector_size);
    }

    return -EINVAL;
}

static struct device_ops ata_ops = {
    NULL, ata_read, ata_write, NULL, NULL, NULL
};

static void ata_init_device(struct ata_port_info *info, uint16_t *identity, size_t i) {
    struct device *device = malloc(sizeof(struct device));
    device->device_number = info->io_base + info->is_slave;
    
    char num[2];
    num[0] = (char) (i + '0');
    num[1] = '\0';
    strcpy(device->name, "hdd");
    strcat(device->name, num);

    device->ops = &ata_ops;
    device->type = S_IFBLK;

    struct ata_device_data *data = malloc(sizeof(struct ata_device_data));
    data->port_info = info;
    data->sector_size = ATA_SECTOR_SIZE;
    data->num_sectors = identity[60] | (identity[61] << 16);
    device->private = data;

    dev_add(device, device->name);
}

#define NUM_POSSIBLE_ATA_DEVICES 8

static struct ata_port_info possible_ata_devices[NUM_POSSIBLE_ATA_DEVICES] = {
    { ATA1_IO_BASE, ATA1_CONTROL_BASE, false },
    { ATA1_IO_BASE, ATA1_CONTROL_BASE, true },
    { ATA2_IO_BASE, ATA2_CONTROL_BASE, false },
    { ATA2_IO_BASE, ATA2_CONTROL_BASE, true },
    { ATA3_IO_BASE, ATA3_CONTROL_BASE, false },
    { ATA3_IO_BASE, ATA3_CONTROL_BASE, true },
    { ATA4_IO_BASE, ATA4_CONTROL_BASE, false },
    { ATA4_IO_BASE, ATA4_CONTROL_BASE, true },
};

void init_ata() {
    for (size_t i = 0; i < NUM_POSSIBLE_ATA_DEVICES; i++) {
        uint16_t buf[ATA_SECTOR_SIZE / sizeof(uint16_t)];
        if (ata_device_exists(&possible_ata_devices[i], buf)) {
            debug_log("Initializing ata device: [ %#.4X, %#.4X, %s ]\n", possible_ata_devices[i].io_base, possible_ata_devices[i].control_base, possible_ata_devices[i].is_slave ? "true" : "false");

            ata_init_device(&possible_ata_devices[i], buf, i);
        }
    }
}