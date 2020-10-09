#include <stdbool.h>

#include <kernel/hal/hal.h>
#include <kernel/hal/hw_device.h>
#include <kernel/hal/isa_driver.h>
#include <kernel/hal/output.h>
#include <kernel/hal/x86_64/drivers/bga.h>
#include <kernel/hal/x86_64/drivers/e1000.h>
#include <kernel/hal/x86_64/drivers/pci.h>
#include <kernel/util/init.h>

static bool pci_device_exists(uint8_t bus, uint8_t slot, uint8_t func) {
    return pci_get_vendor(bus, slot, func) != 0xFFFF;
}

bool pci_config_for_class(uint8_t class_code, uint8_t subclass, struct pci_configuration *config) {
    for (uint16_t bus = 0; bus < PCI_BUS_MAX; bus++) {
        for (uint8_t slot = 0; slot < PCI_SLOT_MAX; slot++) {
            for (uint8_t func = 0; func < PCI_FUNC_MAX; func++) {
                if (pci_device_exists(bus, slot, func)) {
                    pci_read_configuation(bus, slot, func, config);

                    if (config->class_code == class_code && config->subclass == subclass) {
                        return true;
                    }

                    if (func == 0 && !(config->header_type & PCI_MULTI_FUNCTION_FLAG)) {
                        break;
                    }
                }
            }
        }
    }

    return false;
}

void detect_pci_devices(struct hw_device *parent) {
    struct hw_device *device = create_hw_device("PCI Controller", parent, hw_device_id_isa(), NULL);
    device->status = HW_STATUS_ACTIVE;

    for (uint16_t bus = 0; bus < PCI_BUS_MAX; bus++) {
        for (uint8_t slot = 0; slot < PCI_SLOT_MAX; slot++) {
            for (uint8_t func = 0; func < PCI_FUNC_MAX; func++) {
                if (pci_device_exists(bus, slot, func)) {
                    debug_log("Found pci device: [ %u, %u, %u ]\n", bus, slot, func);

                    struct pci_configuration config;
                    pci_read_configuation(bus, slot, func, &config);
                    debug_log("Vendor + Device + Class + Subclass + Type: [ %#X, %#X, %#X, %#X, %#X ]\n", config.vendor_id,
                              config.device_id, config.class_code, config.subclass, config.header_type);

                    if (config.vendor_id == PCI_VENDOR_INTEL && config.device_id == PCI_DEVICE_INTEL_E1000) {
                        init_intel_e1000(&config);
                    }

                    if (config.vendor_id == PCI_VENDOR_BOCHS && config.device_id == PCI_DEVICE_BOCHS_VGA) {
                        if (kernel_use_graphics()) {
                            init_bga(&config);
                        }
                    }

                    if (func == 0 && !(config.header_type & PCI_MULTI_FUNCTION_FLAG)) {
                        break;
                    }
                }
            }
        }
    }
}

static struct isa_driver pci_driver = {
    .name = "x86 PCI Controller (old)",
    .detect_devices = detect_pci_devices,
};

static void init_pci(void) {
    register_isa_driver(&pci_driver);
}
INIT_FUNCTION(init_pci, driver);
