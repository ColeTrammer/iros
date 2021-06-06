#include <kernel/hal/output.h>
#include <kernel/hal/pci.h>
#include <kernel/util/init.h>

#define PCI_PORT_ADDRESS 0xCF8
#define PCI_PORT_DATA    0xCFC

#define PCI_PORT_ADDRESS_ENABLED (1U << 31U)

static uint32_t pci_make_config_address(struct pci_device_location location, uint16_t offset) {
    assert(offset <= UINT8_MAX);
    return PCI_PORT_ADDRESS_ENABLED | (((uint32_t) location.bus) << 16) | (((uint32_t) location.slot) << 11) |
           (((uint32_t) location.func) << 8) | (offset & 0xFF);
}

uint32_t pci_config_read32(struct pci_device_location location, uint16_t offset) {
    assert(offset % 4 == 0);
    uint32_t addr = pci_make_config_address(location, offset);
    outl(PCI_PORT_ADDRESS, addr);
    return inl(PCI_PORT_DATA);
}

void pci_config_write32(struct pci_device_location location, uint16_t offset, uint32_t value) {
    assert(offset % 4 == 0);
    uint32_t addr = pci_make_config_address(location, offset);
    outl(PCI_PORT_ADDRESS, addr);
    outl(PCI_PORT_DATA, value);
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
