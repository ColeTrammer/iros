#include <kernel/hal/pci.h>

static void enumerate_pci_bus(uint8_t bus) {
    for (uint8_t slot = 0; slot < PCI_SLOT_MAX; slot++) {
        for (uint8_t func = 0; func < PCI_FUNC_MAX; func++) {
            struct pci_device_location location = { .bus = bus, .slot = slot, .func = func };
            uint16_t vendor_id = pci_read_vendor_id(location);
            if (vendor_id == PCI_VENDOR_ID_INVALID) {
                break;
            }

            uint16_t device_id = pci_read_device_id(location);
            struct pci_device_info info = pci_read_device_info(location);
            uint8_t header_type = pci_read_header_type(location);
            debug_log("PCI device: [ %#.4X, %#.4X, %u, %u, %u, %u, %#.2X ]\n", vendor_id, device_id, info.class_code, info.subclass_code,
                      info.programming_interface, info.revision_id, header_type);

            if (info.class_code == PCI_CLASS_BRIDGE_DEVICE && info.subclass_code == PCI_BRIDGE_DEVICE_PCI_TO_PCI &&
                (header_type & ~PCI_MULTI_FUNCTION_FLAG) == PCI_HEADER_TYPE_PCI_TO_PCI) {
                enumerate_pci_bus(pci_config_read8(location, PCI_CONFIG_SECONDARY_BUS_NUMBER));
            }

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
        if (pci_read_vendor_id(location) == PCI_VENDOR_ID_INVALID) {
            break;
        }

        enumerate_pci_bus(location.bus);
        location.func++;
    } while (is_multi_function && location.func < PCI_FUNC_MAX);
}
