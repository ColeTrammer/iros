#ifndef _KERNEL_HAL_X86_64_DRIVERS_IDE_CHANNEL_H
#define _KERNEL_HAL_X86_64_DRIVERS_IDE_CHANNEL_H 1

#include <kernel/arch/x86_64/asm_utils.h>
#include <kernel/hal/hw_device.h>
#include <kernel/hal/x86_64/drivers/ata.h>
#include <kernel/hal/x86_64/drivers/ata_drive.h>
#include <kernel/irqs/handlers.h>
#include <kernel/proc/wait_queue.h>
#include <kernel/util/spinlock.h>

struct ide_controller;

struct ide_location {
    uint16_t io_base;
    uint16_t command_base;
    uint16_t ide_bus_master;
};

struct ide_channel {
    struct hw_device hw_device;
    struct ide_location location;
    struct ata_drive *drives[2];
    struct irq_handler irq_handler;
    struct wait_queue wait_queue;
    spinlock_t lock;
    int error_from_irq;
    int current_drive;
    bool can_bus_master;
};

enum ata_wait_result {
    ATA_WAIT_RESULT_SUCCESS,
    ATA_WAIT_RESULT_ERROR,
    ATA_WAIT_RESULT_TIMEOUT,
};

enum ata_wait_result ata_wait_not_busy(struct ide_location location);
void ata_select_drive_with_lba(struct ide_channel *channel, int drive, uint32_t lba);
void ata_setup_prdt(struct ata_drive *drive);
int ata_wait_irq(struct ide_channel *channel);

struct ide_channel *ide_create_channel(struct ide_controller *controller, uint16_t io_base, uint16_t command_base, uint16_t ide_bus_master,
                                       uint8_t irq_line);

static inline void ata_write_device_control(struct ide_location location, uint8_t value) {
    outb(location.command_base + ATA_DEVICE_CONTROL_OFFSET, value);
}

static inline uint8_t ata_read_alt_status(struct ide_location location) {
    return inb(location.command_base + ATA_ALT_STATUS_OFFSET);
}

static inline void ata_do_select_drive(struct ide_location location, bool drive) {
    outb(location.io_base + ATA_DRIVE_OFFSET, ATA_DRIVE_ALWAYS_1 | (drive << ATA_DRIVE_SELECT_BIT));
    io_wait_us(20);
}

static inline void ide_channel_lock(struct ide_channel *channel) {
    spin_lock(&channel->lock);
}

static inline void ide_channel_unlock(struct ide_channel *channel) {
    spin_unlock(&channel->lock);
}

#endif /* _KERNEL_HAL_X86_64_DRIVERS_IDE_CHANNEL_H */
