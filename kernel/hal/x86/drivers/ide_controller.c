#include <stdlib.h>

#include <kernel/arch/x86/asm_utils.h>
#include <kernel/hal/output.h>
#include <kernel/hal/pci_driver.h>
#include <kernel/hal/x86/drivers/ata.h>
#include <kernel/hal/x86/drivers/ide_controller.h>

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
    controller->pci_device.hw_device.status = HW_STATUS_ACTIVE;

    uint32_t channel0_io_base = ide_get_io_base(location, 0, IDE_CONTROLLER_CHANNEL0_DEFAULT_IO_BASE);
    uint32_t channel0_command_base = ide_get_io_base(location, 1, IDE_CONTROLLER_CHANNEL0_DEFAULT_COMMAND_BASE);
    uint32_t channel1_io_base = ide_get_io_base(location, 2, IDE_CONTROLLER_CHANNEL1_DEFAULT_IO_BASE);
    uint32_t channel1_command_base = ide_get_io_base(location, 3, IDE_CONTROLLER_CHANNEL1_DEFAULT_COMMAND_BASE);
    uint32_t ide_bus_master_base = ide_get_io_base(location, 4, 0);

    debug_log("Detected IDE Controller: [ %#.2X, %#.4X, %#.4X, %#.4X, %#.4X, %#.4X ]\n", info.programming_interface, channel0_io_base,
              channel0_command_base, channel1_io_base, channel1_command_base, ide_bus_master_base);

    if (!!(info.programming_interface & IDE_CONTROLLER_IF_BUS_MASTER_SUPPORTED)) {
        pci_enable_bus_mastering(location);
    }

    controller->channels[0] = ide_create_channel(controller, channel0_io_base, channel0_command_base, ide_bus_master_base,
                                                 EXTERNAL_IRQ_OFFSET + IDE_CONTROLLER_IRQ0);
    controller->channels[1] = ide_create_channel(controller, channel1_io_base, channel1_command_base, ide_bus_master_base + 8,
                                                 EXTERNAL_IRQ_OFFSET + IDE_CONTROLLER_IRQ1);

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
