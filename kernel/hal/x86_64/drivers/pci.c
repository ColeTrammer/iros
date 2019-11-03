#include <stdbool.h>

#include <kernel/hal/output.h>
#include <kernel/hal/x86_64/drivers/e1000.h>
#include <kernel/hal/x86_64/drivers/pci.h>

static bool pci_device_exists(uint8_t bus, uint8_t slot) {
    return pci_get_vendor(bus, slot) != 0xFFFF;
}

void init_pci() { 
    for (uint16_t bus = 0; bus < PCI_BUS_MAX; bus++) {
        for (uint8_t slot = 0; slot < PCI_SLOT_MAX; slot++) {
            if (pci_device_exists(bus, slot)) {
                debug_log("Found pci device: [ %u, %u ]\n", bus, slot);

                struct pci_configuration config;
                pci_read_configuation(bus, slot, 0, &config);
                debug_log("Vendor + Device id: [ %#X, %#X ]\n", config.vendor_id, config.device_id);

                if (config.vendor_id == PCI_VENDOR_INTEL && config.device_id == PCI_DEVICE_INTEL_E1000) {
                    init_intel_e1000(&config);
                }
            }
        }
    }
}