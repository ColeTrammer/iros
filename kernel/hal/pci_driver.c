#include <string.h>
#include <kernel/hal/output.h>
#include <kernel/hal/pci.h>
#include <kernel/hal/pci_driver.h>
#include <kernel/util/init.h>

#define PCI_CLASS_BRIDGE 0x06

#define PCI_BRIDGE_HOST       0x00
#define PCI_BRIDGE_PCI_TO_PCI 0x04

static struct pci_device *pci_default_create(struct hw_device *parent, struct pci_device_location location, struct pci_device_id id,
                                             struct pci_device_info info) {
    struct pci_device *device = create_pci_device(location, info);

    if (info.class_code == PCI_CLASS_BRIDGE && info.subclass_code == PCI_BRIDGE_HOST) {
        init_hw_device(&device->hw_device, "PCI Host Bridge", parent, hw_device_id_pci(id), NULL, NULL);

        debug_log("Discovered PCI Host Bridge: [ %#.4X, %#.4X ]\n", id.vendor_id, id.device_id);
        device->is_host_bridge = true;
        enumerate_pci_bus(&device->hw_device, location.func);
        return device;
    }

    if (info.class_code == PCI_CLASS_BRIDGE && info.subclass_code == PCI_BRIDGE_PCI_TO_PCI) {
        init_hw_device(&device->hw_device, "PCI to PCI Bridge", parent, hw_device_id_pci(id), NULL, NULL);

        uint8_t secondary_bus = pci_config_read8(location, PCI_CONFIG_SECONDARY_BUS_NUMBER);
        debug_log("Discovered PCI to PCI Bridge: [ %#.4X, %#.4X, %u ]\n", id.vendor_id, id.device_id, secondary_bus);
        enumerate_pci_bus(&device->hw_device, secondary_bus);
        return device;
    }

    uint8_t header_type = pci_read_header_type(location);
    debug_log("Unknown PCI device: [ %#.4X, %#.4X, %u, %u, %u, %u, %#.2X ]\n", id.vendor_id, id.device_id, info.class_code,
              info.subclass_code, info.programming_interface, info.revision_id, header_type);
    init_hw_device(&device->hw_device, "Unknown PCI Device", parent, hw_device_id_pci(id), NULL, NULL);
    return device;
}

static struct pci_driver_ops pci_default_ops = {
    .create = pci_default_create,
};

static struct pci_driver pci_default_driver = {
    .name = "PCI Default Driver",
    .ops = &pci_default_ops,
    .is_default = true,
};

PCI_DRIVER_INIT(pci_default_driver);

static struct list_node pci_drivers = INIT_LIST(pci_drivers);

void pci_register_driver(struct pci_driver *driver) {
    debug_log("Registering PCI Driver: [ %s ]\n", driver->name);
    list_append(&pci_drivers, &driver->list);
}

void pci_unregister_driver(struct pci_driver *driver) {
    debug_log("Unregistering PCI Driver: [ %s ]\n", driver->name);
    list_remove(&driver->list);
}

struct pci_driver *pci_find_driver(struct pci_device_id id, struct pci_device_info) {
    struct pci_driver *default_driver = NULL;
    list_for_each_entry(&pci_drivers, driver, struct pci_driver, list) {
        if (driver->is_default) {
            default_driver = driver;
        }

        for (size_t i = 0; i < driver->device_id_count; i++) {
            if (memcmp(&id, &driver->device_id_table[i], sizeof(id)) == 0) {
                return driver;
            }
        }
    }
    return default_driver;
}
