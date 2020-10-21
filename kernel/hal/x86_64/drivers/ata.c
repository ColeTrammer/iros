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
#include <kernel/hal/block.h>
#include <kernel/hal/hal.h>
#include <kernel/hal/isa_driver.h>
#include <kernel/hal/output.h>
#include <kernel/hal/processor.h>
#include <kernel/hal/x86_64/drivers/ata.h>
#include <kernel/hal/x86_64/drivers/pci.h>
#include <kernel/hal/x86_64/drivers/pic.h>
#include <kernel/hal/x86_64/drivers/serial.h>
#include <kernel/irqs/handlers.h>
#include <kernel/mem/phys_page.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/proc/task.h>
#include <kernel/util/init.h>

#define DMA_BUFFER_PAGES 1

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

static void ata_wait_irq(struct ata_device_data *data) {
    wait_simple(get_current_task(), &data->wait_queue);
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

static ssize_t ata_read_sectors_dma(struct block_device *self, void *buffer, blkcnt_t n, off_t sector_offset) {
    struct ata_device_data *data = self->private_data;
    size_t sectors = n;
    if (n == 0) {
        return 0;
    } else if (n == 16 * PAGE_SIZE / self->block_size) {
        sectors = 0;
    } else if (n > 16 * PAGE_SIZE / self->block_size) {
        return -EINVAL;
    }

    data->prdt[0].phys_addr = (uint32_t) get_phys_addr((uintptr_t) data->dma_region->start);
    data->prdt[0].size = (uint16_t)(self->block_size * sectors);

    // DMA bus mastering specifc
    outb(data->port_info->bus_mastering_base, 0);
    outl(data->port_info->bus_mastering_base + 4, (uint32_t) get_phys_addr((uintptr_t) data->prdt));
    outb(data->port_info->bus_mastering_base + 2, inb(data->port_info->bus_mastering_base + 2) | 0x06);
    outb(data->port_info->bus_mastering_base, 0x8);

    ata_wait_not_busy(data->port_info);

    ata_setup_registers_dma(data->port_info, sector_offset, sectors);

    ata_full_wait(data->port_info);

    ata_set_command(data->port_info, ATA_COMMAND_READ_DMA);
    io_wait();

    // Start bus master (Disable interrupts so that the irq doesn't come before we sleep)
    uint64_t save = disable_interrupts_save();
    outb(data->port_info->bus_mastering_base, 0x9);
    ata_wait_irq(data);
    interrupts_restore(save);

    memcpy(buffer, (void *) data->dma_region->start, self->block_size * n);

    outb(data->port_info->bus_mastering_base + 2, inb(data->port_info->bus_mastering_base + 2) | 0x06);
    return n;
}

static ssize_t ata_read_sectors(struct block_device *self, void *buffer, blkcnt_t n, off_t sector_offset) {
    struct ata_device_data *data = self->private_data;
    if (n == 0) {
        return 0;
    } else if (n > 255) {
        return -EINVAL;
    } else if (n == 255) {
        n = 0;
    }

    uint64_t save = disable_interrupts_save();
    ata_wait_not_busy(data->port_info);

    ata_setup_registers_pio(data->port_info, sector_offset, n);
    io_wait();

    ata_wait_ready(data->port_info);
    ata_set_command(data->port_info, ATA_COMMAND_READ);

    uint16_t *buf = (uint16_t *) buffer;

    for (blkcnt_t j = 0; j < n; j++) {
        ata_wait(data->port_info);
        ata_wait_not_busy(data->port_info);

        if (ata_read_status(data->port_info) & ATA_STATUS_ERR || !(ata_read_status(data->port_info) & ATA_STATUS_DRQ)) {
            return -EIO;
        }

        for (size_t i = 0; i < self->block_size / sizeof(uint16_t); i++) {
            buf[j * self->block_size / sizeof(uint16_t) + i] = ata_read_word(data->port_info);
        }
    }

    interrupts_restore(save);
    return n;
}

static struct phys_page *ata_read_page_dma(struct block_device *self, off_t sector_offset) {
    struct ata_device_data *data = self->private_data;
    blkcnt_t sectors = PAGE_SIZE / self->block_size;

    struct phys_page *page = block_allocate_phys_page(self);
    if (!page) {
        return NULL;
    }
    page->block_offset = sector_offset;

    // Read directly into the phys_page
    data->prdt[0].phys_addr = page->phys_addr;
    data->prdt[0].size = PAGE_SIZE;

    // DMA bus mastering specifc
    outb(data->port_info->bus_mastering_base, 0);
    outl(data->port_info->bus_mastering_base + 4, (uint32_t) get_phys_addr((uintptr_t) data->prdt));
    outb(data->port_info->bus_mastering_base + 2, inb(data->port_info->bus_mastering_base + 2) | 0x06);
    outb(data->port_info->bus_mastering_base, 0x8);

    ata_wait_not_busy(data->port_info);

    ata_setup_registers_dma(data->port_info, sector_offset, sectors);

    ata_full_wait(data->port_info);

    ata_set_command(data->port_info, ATA_COMMAND_READ_DMA);
    io_wait();

    // Start bus master (Disable interrupts so that the irq doesn't come before we sleep)
    uint64_t save = disable_interrupts_save();
    outb(data->port_info->bus_mastering_base, 0x9);
    ata_wait_irq(data);
    interrupts_restore(save);

    outb(data->port_info->bus_mastering_base + 2, inb(data->port_info->bus_mastering_base + 2) | 0x06);
    return page;
}

static ssize_t ata_write_sectors_dma(struct block_device *self, const void *buffer, blkcnt_t n, off_t sector_offset) {
    struct ata_device_data *data = self->private_data;
    size_t sectors = n;
    if (n == 0) {
        return 0;
    } else if (n == 16 * PAGE_SIZE / self->block_size) {
        sectors = 0;
    } else if (n > 16 * PAGE_SIZE / self->block_size) {
        return -EINVAL;
    }

    data->prdt[0].phys_addr = (uint32_t) get_phys_addr(data->dma_region->start);
    data->prdt[0].size = (uint16_t)(self->block_size * sectors);

    memcpy((void *) data->dma_region->start, buffer, n * self->block_size);

    // DMA bus mastering specifc
    outb(data->port_info->bus_mastering_base, 0);
    outl(data->port_info->bus_mastering_base + 4, (uint32_t) get_phys_addr((uintptr_t) data->prdt));
    outb(data->port_info->bus_mastering_base + 2, inb(data->port_info->bus_mastering_base + 2) | 0x06);
    outb(data->port_info->bus_mastering_base, 0x8);

    ata_wait_not_busy(data->port_info);

    ata_setup_registers_dma(data->port_info, sector_offset, n);

    ata_full_wait(data->port_info);

    ata_set_command(data->port_info, ATA_COMMAND_WRITE_DMA);
    io_wait();

    // Start bus master (Disable interrupts so that the irq doesn't come before we sleep)
    uint64_t save = disable_interrupts_save();
    outb(data->port_info->bus_mastering_base, 0x1);
    ata_wait_irq(data);
    interrupts_restore(save);

    outb(data->port_info->bus_mastering_base + 2, inb(data->port_info->bus_mastering_base + 2) | 0x06);
    return n;
}

static ssize_t ata_write_sectors(struct block_device *self, const void *buffer, blkcnt_t n, off_t sector_offset) {
    struct ata_device_data *data = self->private_data;
    if (n == 0) {
        return 0;
    } else if (n > 255) {
        return -EINVAL;
    } else if (n == 255) {
        n = 0;
    }

    uint64_t save = disable_interrupts_save();
    ata_wait_not_busy(data->port_info);

    ata_setup_registers_pio(data->port_info, sector_offset, n);
    io_wait();

    ata_wait_ready(data->port_info);
    ata_set_command(data->port_info, ATA_COMMAND_WRITE);

    const uint16_t *buf = (const uint16_t *) buffer;

    for (blkcnt_t j = 0; j < n; j++) {
        ata_wait(data->port_info);
        ata_wait_not_busy(data->port_info);

        if (ata_get_error(data->port_info)) {
            /* Might have to also reset the drive */
            return -EIO;
        }

        for (size_t i = 0; i < self->block_size / sizeof(uint16_t); i++) {
            ata_write_word(data->port_info, buf[j * self->block_size / sizeof(uint16_t) + i]);
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

    interrupts_restore(save);
    return n;
}

static int ata_sync_page_dma(struct block_device *self, struct phys_page *page) {
    struct ata_device_data *data = self->private_data;
    blkcnt_t sectors = PAGE_SIZE / self->block_size;

    // Write directly from the physical page.
    data->prdt[0].phys_addr = page->phys_addr;
    data->prdt[0].size = PAGE_SIZE;

    // DMA bus mastering specifc
    outb(data->port_info->bus_mastering_base, 0);
    outl(data->port_info->bus_mastering_base + 4, (uint32_t) get_phys_addr((uintptr_t) data->prdt));
    outb(data->port_info->bus_mastering_base + 2, inb(data->port_info->bus_mastering_base + 2) | 0x06);
    outb(data->port_info->bus_mastering_base, 0x8);

    ata_wait_not_busy(data->port_info);

    ata_setup_registers_dma(data->port_info, page->block_offset, sectors);

    ata_full_wait(data->port_info);

    ata_set_command(data->port_info, ATA_COMMAND_WRITE_DMA);
    io_wait();

    // Start bus master (Disable interrupts so that the irq doesn't come before we sleep)
    uint64_t save = disable_interrupts_save();
    outb(data->port_info->bus_mastering_base, 0x1);
    ata_wait_irq(data);
    interrupts_restore(save);

    outb(data->port_info->bus_mastering_base + 2, inb(data->port_info->bus_mastering_base + 2) | 0x06);
    return 0;
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

static struct block_device_ops ata_ops = {
    .read = ata_read_sectors,
    .write = ata_write_sectors,
    .read_page = block_generic_read_page,
    .sync_page = block_generic_sync_page,
};

static struct block_device_ops ata_dma_ops = {
    .read = ata_read_sectors_dma,
    .write = ata_write_sectors_dma,
    .read_page = ata_read_page_dma,
    .sync_page = ata_sync_page_dma,
};

static void ata_handle_irq(struct irq_context *context) {
    struct ata_device_data *data = context->closure;

    uint8_t status = inb(data->port_info->io_base + ATA_STATUS_OFFSET);
    if (status & 1) {
        debug_log("ata error: [ %u ]\n", status);
    }

    wake_up_all(&data->wait_queue);
}

static void ata_init_device(struct hw_device *parent, struct ata_port_info *info, uint16_t *identity, size_t i) {
    struct ata_device_data *data = malloc(sizeof(struct ata_device_data));
    init_hw_device(&data->hw_device, "PATA Drive", parent, hw_device_id_isa(), NULL, NULL);
    data->hw_device.status = HW_STATUS_ACTIVE;
    data->port_info = info;
    init_wait_queue(&data->wait_queue);

    dev_t device_number = 0x00500 + 16 * i;
    blksize_t block_size = ATA_SECTOR_SIZE;
    blkcnt_t block_count = identity[60] | (identity[61] << 16);
    struct block_device_ops *op = &ata_ops;

    struct pci_configuration pci_config;
    if (pci_config_for_class(PCI_CLASS_MASS_STORAGE, PCI_SUBCLASS_IDE_CONTROLLER, &pci_config)) {
        pci_enable_bus_mastering(&pci_config);
        uint32_t base = pci_config.bar[4] & 0xFFFC;
        data->port_info->bus_mastering_base = base;
        data->port_info->use_dma = true;
        data->dma_region = vm_allocate_dma_region(PAGE_SIZE * DMA_BUFFER_PAGES);
        op = &ata_dma_ops;

        register_irq_handler(create_irq_handler(ata_handle_irq, IRQ_HANDLER_EXTERNAL, data), info->irq + EXTERNAL_IRQ_OFFSET);

        debug_log("found pic for ata (so will use dma): [ %#.8X ]\n", base);
    }

    struct block_device *block_device = create_block_device(block_count, block_size, block_device_info_none(BLOCK_TYPE_DISK), op, data);
    char name[16];
    snprintf(name, sizeof(name), "sd%c", 'a' + (char) i);
    block_register_device(block_device, name, device_number);
}

#define NUM_POSSIBLE_ATA_DEVICES 8

static struct ata_port_info possible_ata_devices[NUM_POSSIBLE_ATA_DEVICES] = {
    { ATA1_IO_BASE, ATA1_CONTROL_BASE, ATA1_IRQ, 0, false, false }, { ATA1_IO_BASE, ATA1_CONTROL_BASE, 0, ATA1_IRQ, true, false },
    { ATA2_IO_BASE, ATA2_CONTROL_BASE, ATA2_IRQ, 0, false, false }, { ATA2_IO_BASE, ATA2_CONTROL_BASE, 0, ATA2_IRQ, true, false },
    { ATA3_IO_BASE, ATA3_CONTROL_BASE, ATA3_IRQ, 0, false, false }, { ATA3_IO_BASE, ATA3_CONTROL_BASE, 0, ATA3_IRQ, true, false },
    { ATA4_IO_BASE, ATA4_CONTROL_BASE, ATA4_IRQ, 0, false, false }, { ATA4_IO_BASE, ATA4_CONTROL_BASE, 0, ATA4_IRQ, true, false },
};

static void detect_ata(struct hw_device *parent) {
    for (size_t i = 0; i < NUM_POSSIBLE_ATA_DEVICES; i++) {
        uint16_t buf[ATA_SECTOR_SIZE / sizeof(uint16_t)];
        if (ata_device_exists(&possible_ata_devices[i], buf)) {
            debug_log("Initializing ata device: [ %#.4X, %#.4X, %d, %s ]\n", possible_ata_devices[i].io_base,
                      possible_ata_devices[i].control_base, possible_ata_devices[i].irq,
                      possible_ata_devices[i].is_slave ? "true" : "false");

            ata_init_device(parent, &possible_ata_devices[i], buf, i);
        }
    }
}

static struct isa_driver ata_driver = {
    .name = "x86 PATA IDE",
    .detect_devices = detect_ata,
};

static void init_ata(void) {
    register_isa_driver(&ata_driver);
}
INIT_FUNCTION(init_ata, driver);
