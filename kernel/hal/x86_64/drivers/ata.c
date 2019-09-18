#include <stdint.h>

#include <kernel/hal/output.h>
#include <kernel/hal/x86_64/drivers/ata.h>
#include <kernel/arch/x86_64/asm_utils.h>

void init_ata() {
    uint8_t init_status = inb(ATA1_IO_BASE + ATA_STATUS_OFFSET);
    debug_log("Ata initial status: [ %#.2X ]\n", init_status);

    while (inb(ATA1_IO_BASE + ATA_STATUS_OFFSET) & ATA_STATUS_BSY);

    outb(ATA1_IO_BASE + ATA_DEVICE_OFFSET, 0xA0);

    inb(ATA1_CONTROL_BASE);
    inb(ATA1_CONTROL_BASE);
    inb(ATA1_CONTROL_BASE);
    inb(ATA1_CONTROL_BASE);
    uint8_t cl = inb(ATA1_IO_BASE + ATA_CYLINDER_LOW_OFFSET);
    uint8_t ch = inb(ATA1_IO_BASE + ATA_CYLINDER_HIGH_OFFSET);

    debug_log("Ata device: [ %u, %u ]\n", cl, ch);

    debug_log("Ata status: [ %#.2X ]\n", inb(ATA1_IO_BASE + ATA_STATUS_OFFSET));

    while (inb(ATA1_IO_BASE + ATA_STATUS_OFFSET) & ATA_STATUS_BSY);

    debug_log("Ata status: [ %#.2X ]\n", inb(ATA1_IO_BASE + ATA_STATUS_OFFSET));


    outb(ATA1_IO_BASE + ATA_DEVICE_OFFSET, ATA_MASTER);

    outb(ATA1_IO_BASE + ATA_SECTOR_COUNT_OFFSET, 1);

    outb(ATA1_IO_BASE + ATA_SECTOR_NUMBER_OFFSET, 0);
    outb(ATA1_IO_BASE + ATA_CYLINDER_LOW_OFFSET, 0);
    outb(ATA1_IO_BASE + ATA_CYLINDER_HIGH_OFFSET, 0);

    outb(ATA1_CONTROL_BASE, 0x08);

    debug_log("Ata status: [ %#.2X ]\n", inb(ATA1_IO_BASE + ATA_STATUS_OFFSET));

    while (!(inb(ATA1_IO_BASE + ATA_STATUS_OFFSET) & ATA_STATUS_RDY));

    debug_log("Ata status: [ %#.2X ]\n", inb(ATA1_IO_BASE + ATA_STATUS_OFFSET));

    outb(ATA1_IO_BASE + ATA_COMMAND_OFFSET, ATA_COMMAND_READ);

    for (size_t j = 0; j < 1; j++) {
        for (size_t i = 0; i < 4; i++) {
            inb(ATA1_CONTROL_BASE);
        }

        debug_log("Ata status: [ %#.2X ]\n", inb(ATA1_IO_BASE + ATA_STATUS_OFFSET));

        while (inb(ATA1_IO_BASE + ATA_STATUS_OFFSET) & ATA_STATUS_BSY);

        debug_log("Ata status: [ %#.2X ]\n", inb(ATA1_IO_BASE + ATA_STATUS_OFFSET));

        for (size_t i = 0; i < 256; i++) {
            debug_log("Ata status: [ %#.2X ]\n", inb(ATA1_IO_BASE + ATA_STATUS_OFFSET));
            debug_log("Recieved data: [ %#.4X ]\n", inw(ATA1_IO_BASE));
        }
    }
}