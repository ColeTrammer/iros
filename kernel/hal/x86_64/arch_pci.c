#include <stdbool.h>

#include <kernel/hal/hal.h>
#include <kernel/hal/hw_device.h>
#include <kernel/hal/isa_driver.h>
#include <kernel/hal/output.h>
#include <kernel/hal/x86_64/drivers/bga.h>
#include <kernel/hal/x86_64/drivers/e1000.h>
#include <kernel/hal/x86_64/drivers/pci.h>
#include <kernel/util/init.h>

uint32_t pci_config_read32(struct pci_device_location location, uint16_t offset) {
    assert(offset % 4 == 0);
    uint32_t addr = pci_make_config_address(location.bus, location.slot, location.func, offset);
    outl(PCI_CONFIG_ADDRESS, addr);
    return inl(PCI_CONFIG_DATA);
}

void pci_config_write32(struct pci_device_location location, uint16_t offset, uint32_t value) {
    assert(offset % 4 == 0);
    uint32_t addr = pci_make_config_address(location.bus, location.slot, location.func, offset);
    outl(PCI_CONFIG_ADDRESS, addr);
    outl(PCI_CONFIG_DATA, value);
}

static uint32_t mask8_array[4] = {
    0xFFFFFF00,
    0xFFFF00FF,
    0xFF00FFFF,
    0x00FFFFFF,
};

static uint32_t mask16_array[4] = {
    0xFFFF0000,
    0,
    0x0000FFFF,
    0,
};

static uint32_t shift_array[4] = { 0, 8, 16, 24 };

void pci_config_write8(struct pci_device_location location, uint16_t offset, uint8_t value) {
    uint32_t old_value = pci_config_read32(location, offset & ~3);
    uint32_t mask = mask8_array[offset & 3];
    uint32_t shift = shift_array[offset & 3];

    old_value &= mask;
    old_value |= value << shift;
    pci_config_write32(location, offset & ~3, old_value);
}

uint8_t pci_config_read8(struct pci_device_location location, uint16_t offset) {
    uint32_t value = pci_config_read32(location, offset & ~3);
    return (value & ~mask8_array[offset & 3]) >> shift_array[offset & 3];
}

void pci_config_write16(struct pci_device_location location, uint16_t offset, uint16_t value) {
    assert(offset % 2 == 0);
    uint32_t old_value = pci_config_read32(location, offset & ~3);
    uint32_t mask = mask16_array[offset & 3];
    uint32_t shift = shift_array[offset & 3];

    old_value &= mask;
    old_value |= value << shift;
    pci_config_write32(location, offset & ~3, old_value);
}

uint16_t pci_config_read16(struct pci_device_location location, uint16_t offset) {
    uint32_t value = pci_config_read32(location, offset & ~3);
    return (value & ~mask16_array[offset & 3]) >> shift_array[offset & 3];
}

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
