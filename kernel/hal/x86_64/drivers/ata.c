#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <kernel/arch/x86_64/asm_utils.h>
#include <kernel/fs/dev.h>
#include <kernel/hal/hal.h>
#include <kernel/hal/output.h>
#include <kernel/hal/x86_64/drivers/ata.h>
#include <kernel/hal/x86_64/drivers/pci.h>
#include <kernel/hal/x86_64/drivers/pic.h>
#include <kernel/hal/x86_64/drivers/serial.h>
#include <kernel/proc/task.h>

#define DMA_BUFFER_PAGES 15

static void ata_wait(struct ata_port_info *info) {
    for (size_t i = 0; i < 4; i++) {
        inb(info->control_base + ATA_ALT_STATUS_OFFSET);
    }
}

static void ata_full_wait(struct ata_port_info *info) {
    for (;;) {
        int status = inb(info->io_base + ATA_STATUS_OFFSET);
        if (!(status & ATA_STATUS_BSY) && (status & ATA_STATUS_RDY))
            break;
    }
}

static void ata_wait_not_busy(struct ata_port_info *info) {
    while (inb(info->io_base + ATA_STATUS_OFFSET) & ATA_STATUS_BSY)
        ;
}

static void ata_wait_ready(struct ata_port_info *info) {
    while (!(inb(info->io_base + ATA_STATUS_OFFSET) & ATA_STATUS_RDY))
        ;
}

static void ata_wait_irq(struct ata_device_data *data __attribute__((unused))) {
    if (data->waiter && !data->waiter->kernel_task) {
        proc_block_custom(data->waiter);
    } else {
        // Terrible hack b/c we can't wait on irq's before scheduling starts.
        // This is seriously the worst, as it is entirely possible this wait
        // time is not long enough. Not only that, but also errors will go
        // uncheck. There may be a way to poll this with the status register,
        // but it can't currently be determined b/c irqs as a whole are broken.
        for (int i = 0; i < 10000; i++) {
            io_wait();
        }
    }
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

static void ata_setup_registers_dma(struct ata_port_info *info, uint32_t offset, uint8_t sectors) {
    outb(info->control_base + ATA_ALT_STATUS_OFFSET, 0);
    outb(info->io_base + ATA_DRIVE_OFFSET, 0xE0 | (info->is_slave << 4) | ((offset >> 24) & 0xF));
    io_wait();
    outb(info->io_base + ATA_FEATURES_OFFSET, 0);

    outb(info->io_base + ATA_SECTOR_COUNT_OFFSET, 0);
    outb(info->io_base + ATA_SECTOR_NUMBER_OFFSET, 0);
    outb(info->io_base + ATA_CYLINDER_LOW_OFFSET, 0);
    outb(info->io_base + ATA_CYLINDER_HIGH_OFFSET, 0);

    outb(info->io_base + ATA_SECTOR_COUNT_OFFSET, sectors);
    outb(info->io_base + ATA_SECTOR_NUMBER_OFFSET, offset & 0xFF);
    outb(info->io_base + ATA_CYLINDER_LOW_OFFSET, (offset >> 8) & 0xFF);
    outb(info->io_base + ATA_CYLINDER_HIGH_OFFSET, (offset >> 16) & 0xFF);
}

static void ata_setup_registers_pio(struct ata_port_info *info, uint32_t offset, uint8_t sectors) {
    outb(info->io_base + ATA_SECTOR_COUNT_OFFSET, sectors);
    outb(info->io_base + ATA_SECTOR_NUMBER_OFFSET, offset & 0xFF);
    outb(info->io_base + ATA_CYLINDER_LOW_OFFSET, (offset >> 8) & 0xFF);
    outb(info->io_base + ATA_CYLINDER_HIGH_OFFSET, (offset >> 16) & 0xFF);
    outb(info->io_base + ATA_DRIVE_OFFSET, 0xE0 | (info->is_slave << 4) | ((offset >> 24) & 0xF));
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

static ssize_t ata_read_sectors_dma(struct ata_device_data *data, size_t offset, void *buffer, size_t n) {
    size_t sectors = n;
    if (n == 0) {
        return 0;
    } else if (n == 16 * PAGE_SIZE / data->sector_size) {
        sectors = 0;
    } else if (n > 16 * PAGE_SIZE / data->sector_size) {
        return -EINVAL;
    }

    data->prdt[0].phys_addr = (uint32_t) get_phys_addr((uintptr_t) data->dma_page);
    data->prdt[0].size = (uint16_t)(data->sector_size * sectors);

    // DMA bus mastering specifc
    outb(data->port_info->bus_mastering_base, 0);
    outl(data->port_info->bus_mastering_base + 4, (uint32_t) get_phys_addr((uintptr_t) data->prdt));
    outb(data->port_info->bus_mastering_base + 2, inb(data->port_info->bus_mastering_base + 2) | 0x06);
    outb(data->port_info->bus_mastering_base, 0x8);

    ata_wait_not_busy(data->port_info);

    ata_setup_registers_dma(data->port_info, offset, sectors);

    ata_full_wait(data->port_info);

    ata_set_command(data->port_info, ATA_COMMAND_READ_DMA);
    io_wait();

    // Begin bus master
    outb(data->port_info->bus_mastering_base, 0x9);

    ata_wait_irq(data);

    memcpy(buffer, data->dma_page, data->sector_size * n);

    outb(data->port_info->bus_mastering_base + 2, inb(data->port_info->bus_mastering_base + 2) | 0x06);
    return data->sector_size * n;
}

static ssize_t ata_read_sectors(struct ata_device_data *data, size_t offset, void *buffer, size_t n) {
    if (n == 0) {
        return 0;
    } else if (n > 255) {
        return -EINVAL;
    } else if (n == 255) {
        n = 0;
    }

    ata_wait_not_busy(data->port_info);

    ata_setup_registers_pio(data->port_info, offset, n);
    io_wait();

    ata_wait_ready(data->port_info);
    ata_set_command(data->port_info, ATA_COMMAND_READ);

    uint16_t *buf = (uint16_t *) buffer;

    for (size_t j = 0; j < n; j++) {
        ata_wait(data->port_info);
        ata_wait_not_busy(data->port_info);

        if (ata_read_status(data->port_info) & ATA_STATUS_ERR || !(ata_read_status(data->port_info) & ATA_STATUS_DRQ)) {
            return -EIO;
        }

        for (size_t i = 0; i < data->sector_size / sizeof(uint16_t); i++) {
            buf[j * data->sector_size / sizeof(uint16_t) + i] = ata_read_word(data->port_info);
        }
    }

    return n * data->sector_size;
}

static ssize_t ata_write_sectors_dma(struct ata_device_data *data, size_t offset, const void *buffer, size_t n) {
    size_t sectors = n;
    if (n == 0) {
        return 0;
    } else if (n == 16 * PAGE_SIZE / data->sector_size) {
        sectors = 0;
    } else if (n > 16 * PAGE_SIZE / data->sector_size) {
        return -EINVAL;
    }

    data->prdt[0].phys_addr = (uint32_t) get_phys_addr((uintptr_t) data->dma_page);
    data->prdt[0].size = (uint16_t)(data->sector_size * sectors);

    memcpy(data->dma_page, buffer, n * data->sector_size);

    // DMA bus mastering specifc
    outb(data->port_info->bus_mastering_base, 0);
    outl(data->port_info->bus_mastering_base + 4, (uint32_t) get_phys_addr((uintptr_t) data->prdt));
    outb(data->port_info->bus_mastering_base + 2, inb(data->port_info->bus_mastering_base + 2) | 0x06);
    outb(data->port_info->bus_mastering_base, 0x8);

    ata_wait_not_busy(data->port_info);

    ata_setup_registers_dma(data->port_info, offset, n);

    ata_full_wait(data->port_info);

    ata_set_command(data->port_info, ATA_COMMAND_WRITE_DMA);
    io_wait();

    // Start bus master
    outb(data->port_info->bus_mastering_base, 0x1);

    ata_wait_irq(data);

    outb(data->port_info->bus_mastering_base + 2, inb(data->port_info->bus_mastering_base + 2) | 0x06);
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

    ata_wait_not_busy(data->port_info);

    ata_setup_registers_pio(data->port_info, offset, n);
    io_wait();

    ata_wait_ready(data->port_info);
    ata_set_command(data->port_info, ATA_COMMAND_WRITE);

    const uint16_t *buf = (const uint16_t *) buffer;

    for (size_t j = 0; j < n; j++) {
        ata_wait(data->port_info);
        ata_wait_not_busy(data->port_info);

        if (ata_get_error(data->port_info)) {
            /* Might have to also reset the drive */
            return -EIO;
        }

        for (size_t i = 0; i < data->sector_size / sizeof(uint16_t); i++) {
            ata_write_word(data->port_info, buf[j * data->sector_size / sizeof(uint16_t) + i]);
        }
    }

    /* Clear cache after writing */
    ata_wait_not_busy(data->port_info);
    ata_set_command(data->port_info, ATA_COMMAND_CACHE_FLUSH);
    ata_wait_not_busy(data->port_info);

    if (ata_get_error(data->port_info)) {
        /* Might have to also reset the drive */
        return -EIO;
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

static ssize_t ata_read(struct device *device, off_t offset, void *buffer, size_t n) {
    struct ata_device_data *data = device->private;

    if (n % data->sector_size == 0 && offset % data->sector_size == 0) {
        size_t num_sectors_to_read = n / data->sector_size;
        size_t num_sectors = DMA_BUFFER_PAGES * PAGE_SIZE / data->sector_size;
        ssize_t read = 0;

        spin_lock(&device->inode->lock);
        assert(!data->waiter);
        data->waiter = get_current_task();
        spin_unlock(&device->inode->lock);

        for (size_t i = 0; i < num_sectors_to_read; i += num_sectors) {
            i = MIN(i, num_sectors_to_read);
            size_t to_read = MIN(num_sectors, num_sectors_to_read - i);
            ssize_t ret = (data->port_info->use_dma ? ata_read_sectors_dma : ata_read_sectors)(
                data, (offset / data->sector_size), (void *) (((uintptr_t) buffer) + (i * data->sector_size)), to_read);
            if (ret != (ssize_t)(to_read * data->sector_size)) {
                if (ret < 0) {
                    read = ret;
                    goto finsih_ata_read;
                }

                offset += ret;
                goto finsih_ata_read;
            }

            read += ret;
            offset += ret;
        }

    finsih_ata_read:
        spin_lock(&device->inode->lock);
        data->waiter = NULL;
        spin_unlock(&device->inode->lock);
        return read;
    }

    return -EINVAL;
}

static ssize_t ata_write(struct device *device, off_t offset, const void *buffer, size_t n) {
    struct ata_device_data *data = device->private;

    if (n % data->sector_size == 0 && offset % data->sector_size == 0) {
        size_t num_sectors_to_write = n / data->sector_size;
        size_t num_sectors = DMA_BUFFER_PAGES * PAGE_SIZE / data->sector_size;
        ssize_t written = 0;

        spin_lock(&device->inode->lock);
        assert(!data->waiter);
        data->waiter = get_current_task();
        spin_unlock(&device->inode->lock);

        for (size_t i = 0; i < num_sectors_to_write; i += num_sectors) {
            i = MIN(i, num_sectors_to_write);
            size_t to_write = MIN(num_sectors, num_sectors_to_write - i);
            ssize_t ret = (data->port_info->use_dma ? ata_write_sectors_dma : ata_write_sectors)(
                data, (offset / data->sector_size), (const void *) (((uintptr_t) buffer) + (i * data->sector_size)), to_write);
            if (ret != (ssize_t)(to_write * data->sector_size)) {
                if (ret < 0) {
                    written = ret;
                    goto finsih_ata_write;
                }

                offset += ret;
                goto finsih_ata_write;
            }

            written += ret;
            offset += ret;
        }

    finsih_ata_write:
        spin_lock(&device->inode->lock);
        data->waiter = NULL;
        spin_unlock(&device->inode->lock);
        return written;
    }

    return -EINVAL;
}

static struct device_ops ata_ops = { NULL, ata_read, ata_write, NULL, NULL, NULL, NULL, NULL, NULL };

static void __attribute__((unused)) ata_handle_irq(struct ata_device_data *data) {
    uint8_t status = inb(data->port_info->io_base + ATA_STATUS_OFFSET);
    if (status & 1) {
        debug_log("ata error: [ %u ]\n", status);
    }

    if (data->waiter) {
        data->waiter->sched_state = RUNNING_UNINTERRUPTIBLE;
    }
}

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
    data->waiter = NULL;
    device->private = data;

    struct pci_configuration pci_config;
    if (pci_config_for_class(PCI_CLASS_MASS_STORAGE, PCI_SUBCLASS_IDE_CONTROLLER, &pci_config)) {
        pci_enable_bus_mastering(&pci_config);
        uint32_t base = pci_config.bar[4] & 0xFFFC;
        data->port_info->bus_mastering_base = base;
        data->port_info->use_dma = true;
        data->dma_page = aligned_alloc(PAGE_SIZE, DMA_BUFFER_PAGES * PAGE_SIZE);

        if (!is_irq_line_registered(info->irq)) {
            register_irq_line_handler((void (*)(void *)) ata_handle_irq, info->irq, data, true);
        }

        debug_log("found pic for ata (so will use dma): [ %#.8X ]\n", base);
    }

    dev_add(device, device->name);
}

#define NUM_POSSIBLE_ATA_DEVICES 8

static struct ata_port_info possible_ata_devices[NUM_POSSIBLE_ATA_DEVICES] = {
    { ATA1_IO_BASE, ATA1_CONTROL_BASE, ATA1_IRQ, 0, false, false }, { ATA1_IO_BASE, ATA1_CONTROL_BASE, 0, ATA1_IRQ, true, false },
    { ATA2_IO_BASE, ATA2_CONTROL_BASE, ATA2_IRQ, 0, false, false }, { ATA2_IO_BASE, ATA2_CONTROL_BASE, 0, ATA2_IRQ, true, false },
    { ATA3_IO_BASE, ATA3_CONTROL_BASE, ATA3_IRQ, 0, false, false }, { ATA3_IO_BASE, ATA3_CONTROL_BASE, 0, ATA3_IRQ, true, false },
    { ATA4_IO_BASE, ATA4_CONTROL_BASE, ATA4_IRQ, 0, false, false }, { ATA4_IO_BASE, ATA4_CONTROL_BASE, 0, ATA4_IRQ, true, false },
};

void init_ata() {
    for (size_t i = 0; i < NUM_POSSIBLE_ATA_DEVICES; i++) {
        uint16_t buf[ATA_SECTOR_SIZE / sizeof(uint16_t)];
        if (ata_device_exists(&possible_ata_devices[i], buf)) {
            debug_log("Initializing ata device: [ %#.4X, %#.4X, %d, %s ]\n", possible_ata_devices[i].io_base,
                      possible_ata_devices[i].control_base, possible_ata_devices[i].irq,
                      possible_ata_devices[i].is_slave ? "true" : "false");

            ata_init_device(&possible_ata_devices[i], buf, i);
        }
    }
}