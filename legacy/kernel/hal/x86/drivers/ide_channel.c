#include <stdlib.h>
#include <sys/types.h>

#include <kernel/hal/x86/drivers/ide_channel.h>
#include <kernel/hal/x86/drivers/ide_controller.h>
#include <kernel/mem/page.h>
#include <kernel/proc/task.h>

enum ata_wait_result ata_wait_not_busy(struct ide_location location) {
    for (int i = 0; i < 2000; i++) {
        uint8_t status = ata_read_alt_status(location);
        if (!!(status & ATA_STATUS_ERR) || !!(status & ATA_STATUS_DF)) {
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

static void ata_clear_bus_master_status(struct ide_location location) {
    uint8_t status = inb(location.ide_bus_master + ATA_BUS_MASTER_STATUS_OFFSET);
    outb(location.ide_bus_master + ATA_BUS_MASTER_STATUS_OFFSET, status | ATA_BUS_MASTER_STATUS_DID_IRQ | ATA_BUS_MASTER_STATUS_ERROR);
}

void ata_setup_prdt(struct ata_drive *drive) {
    outb(drive->channel->location.ide_bus_master + ATA_BUS_MASTER_COMMAND_OFFSET, 0);
    outl(drive->channel->location.ide_bus_master + ATA_BUS_MASTER_PRD_OFFSET, (uint32_t) get_phys_addr((uintptr_t) drive->prdt));
    ata_clear_bus_master_status(drive->channel->location);
}

int ata_wait_irq(struct ide_channel *channel) {
    wait_simple(get_current_task(), &channel->wait_queue);
    if (channel->error_from_irq) {
        channel->error_from_irq = false;
        return -EIO;
    }

    return 0;
}

static bool ide_channel_irq(struct irq_context *context) {
    struct ide_channel *channel = context->closure;

    uint8_t status = inb(channel->location.ide_bus_master + ATA_BUS_MASTER_STATUS_OFFSET);
    if (!(status & ATA_BUS_MASTER_STATUS_DID_IRQ)) {
        return false;
    }

    uint8_t ata_status = inb(channel->location.io_base + ATA_STATUS_OFFSET);
    if (!!(ata_status & ATA_STATUS_ERR) || !!(ata_status & ATA_STATUS_DF) || !!(status & ATA_BUS_MASTER_STATUS_ERROR)) {
        channel->error_from_irq = true;
    } else {
        channel->error_from_irq = false;
    }
    ata_clear_bus_master_status(channel->location);
    wake_up_all(&channel->wait_queue);
    return true;
}

struct ide_channel *ide_create_channel(struct ide_controller *controller, uint16_t io_base, uint16_t command_base, uint16_t ide_bus_master,
                                       uint8_t irq_line) {
    struct ide_channel *channel = calloc(1, sizeof(*channel));
    init_hw_device(&channel->hw_device, "IDE Channel", &controller->pci_device.hw_device, hw_device_id_none(), NULL, NULL);
    channel->hw_device.status = HW_STATUS_ACTIVE;

    channel->location.io_base = io_base;
    channel->location.command_base = command_base;
    channel->location.ide_bus_master = ide_bus_master;
    init_spinlock(&channel->lock);
    init_wait_queue(&channel->wait_queue);
    channel->can_bus_master = !!(controller->pci_device.info.programming_interface & IDE_CONTROLLER_IF_BUS_MASTER_SUPPORTED);

    if (channel->can_bus_master) {
        channel->irq_handler.handler = &ide_channel_irq;
        channel->irq_handler.flags = IRQ_HANDLER_EXTERNAL | IRQ_HANDLER_SHARED;
        channel->irq_handler.closure = channel;
        register_irq_handler(&channel->irq_handler, irq_line);
    }

    debug_log("Created IDE Channel: [ %#.4X, %#.4X, %#.4X ]\n", io_base, command_base, ide_bus_master);

    // Ensure drive 0 will be selected
    channel->current_drive = -1;

    for (int drive = 0; drive < 2; drive++) {
        channel->drives[drive] = ata_detect_drive(channel, drive);
    }

    return channel;
}
