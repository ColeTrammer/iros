#include <stdlib.h>
#include <kernel/hal/output.h>
#include <kernel/hal/pci.h>
#include <kernel/hal/pci_driver.h>

void init_pci_device(struct pci_device *device, struct pci_device_location location, struct pci_device_info info) {
    device->location = location;
    device->info = info;
}

struct pci_device *create_pci_device(struct pci_device_location location, struct pci_device_info info) {
    struct pci_device *device = calloc(1, sizeof(*device));
    init_pci_device(device, location, info);
    return device;
}

static void pci_instantiate_device(struct hw_device *parent, struct pci_device_location location, uint16_t vendor_id) {
    uint16_t device_id = pci_read_device_id(location);
    struct pci_device_id id = (struct pci_device_id) { .vendor_id = vendor_id, .device_id = device_id };
    struct pci_device_info info = pci_read_device_info(location);

    struct pci_driver *driver = pci_find_driver(id, info);
    assert(driver);

    driver->ops->create(parent, location, id, info);
}

void enumerate_pci_bus(struct hw_device *parent, uint8_t bus) {
    // NOTE: Bus 0, Slot 0 contains the Host Bridge, which is already instantiated.
    uint8_t slot_start = bus == 0 ? 1 : 0;
    for (uint8_t slot = slot_start; slot < PCI_SLOT_MAX; slot++) {
        for (uint8_t func = 0; func < PCI_FUNC_MAX; func++) {
            struct pci_device_location location = { .bus = bus, .slot = slot, .func = func };
            uint16_t vendor_id = pci_read_vendor_id(location);
            if (vendor_id == PCI_VENDOR_ID_INVALID) {
                break;
            }

            pci_instantiate_device(parent, location, vendor_id);

            uint8_t header_type = pci_read_header_type(location);
            if (func == 0 && !(header_type & PCI_MULTI_FUNCTION_FLAG)) {
                break;
            }
        }
    }
}

void enumerate_pci_devices() {
    struct pci_device_location location = { 0 };
    bool is_multi_function = !!(pci_read_header_type(location) & PCI_MULTI_FUNCTION_FLAG);
    do {
        uint16_t vendor_id = pci_read_vendor_id(location);
        if (vendor_id == PCI_VENDOR_ID_INVALID) {
            break;
        }

        pci_instantiate_device(root_hw_device(), location, vendor_id);
        location.func++;
    } while (is_multi_function && location.func < PCI_FUNC_MAX);
}
