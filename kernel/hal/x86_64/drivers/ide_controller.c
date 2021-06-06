#include <stdlib.h>

#include <kernel/hal/output.h>
#include <kernel/hal/pci_driver.h>
#include <kernel/hal/x86_64/drivers/ide_controller.h>

static uint16_t ide_get_io_port(struct pci_device_location location, uint8_t bar, uint16_t default_value) {
    uint32_t value = pci_config_read32(location, PCI_CONFIG_BAR(bar)) & ~1;
    if (!value) {
        return default_value;
    }
    return value;
}

static struct pci_device *ide_create(struct hw_device *parent, struct pci_device_location location, struct pci_device_id id,
                                     struct pci_device_info info) {
    struct ide_controller *controller = calloc(1, sizeof(struct ide_controller));
    init_pci_device(&controller->pci_device, location, info);
    init_hw_device(&controller->pci_device.hw_device, "IDE Controller", parent, hw_device_id_pci(id), NULL, NULL);

    uint32_t channel0_io_port = ide_get_io_port(location, 0, IDE_CONTROLLER_CHANNEL0_DEFAULT_IO_PORT);
    uint32_t channel0_command_port = ide_get_io_port(location, 1, IDE_CONTROLLER_CHANNEL0_DEFAULT_COMMAND_PORT);
    uint32_t channel1_io_port = ide_get_io_port(location, 2, IDE_CONTROLLER_CHANNEL1_DEFAULT_IO_PORT);
    uint32_t channel1_command_port = ide_get_io_port(location, 3, IDE_CONTROLLER_CHANNEL1_DEFAULT_COMMAND_PORT);
    uint32_t ide_bus_master_port = ide_get_io_port(location, 4, 0);

    debug_log("Detected IDE Controller: [ %#.2X, %#.4X, %#.4X, %#.4X, %#.4X, %#.4X ]\n", info.programming_interface, channel0_io_port,
              channel0_command_port, channel1_io_port, channel1_command_port, ide_bus_master_port);

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
