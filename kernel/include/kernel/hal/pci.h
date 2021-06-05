#ifndef _KERNEL_HAL_PCI_H
#define _KERNEL_HAL_PCI_H 1

#include <kernel/hal/hw_device.h>

#define PCI_BUS_MAX  256
#define PCI_SLOT_MAX 32
#define PCI_FUNC_MAX 8

#define PCI_MULTI_FUNCTION_FLAG 0x80

#define PCI_CONFIG_VENDOR_ID             0x00
#define PCI_CONFIG_DEVICE_ID             0x02
#define PCI_CONFIG_COMMAND               0x04
#define PCI_CONFIG_STATUS                0x06
#define PCI_CONFIG_REVISION_ID           0x08
#define PCI_CONFIG_PROGRAMMING_INTERFACE 0x09
#define PCI_CONFIG_SUBCLASS              0x0A
#define PCI_CONFIG_CLASS                 0x0B
#define PCI_CONFIG_BAR(x)                (0x10 + 4 * (x))
#define PCI_CONFIG_HEADER_TYPE           0x0E
#define PCI_CONFIG_SECONDARY_BUS_NUMBER  0x19

#define PCI_CLASS_BRIDGE_DEVICE      0x06
#define PCI_BRIDGE_DEVICE_PCI_TO_PCI 0x04

#define PCI_HEADER_TYPE_REGULAR    0x00
#define PCI_HEADER_TYPE_PCI_TO_PCI 0x01

#define PCI_VENDOR_ID_INVALID 0xFFFF

struct pci_device_location {
    uint8_t bus;
    uint8_t slot;
    uint8_t func;
};

struct pci_device_info {
    uint8_t class_code;
    uint8_t subclass_code;
    uint8_t programming_interface;
    uint8_t revision_id;
};

struct pci_device {
    struct hw_device hw_device;
    struct pci_device_location location;
    struct pci_device_info info;
};

void enumerate_pci_devices();

void pci_config_write8(struct pci_device_location location, uint16_t offset, uint8_t value);
void pci_config_write16(struct pci_device_location location, uint16_t offset, uint16_t value);
void pci_config_write32(struct pci_device_location location, uint16_t offset, uint32_t value);

uint8_t pci_config_read8(struct pci_device_location location, uint16_t offset);
uint16_t pci_config_read16(struct pci_device_location location, uint16_t offset);
uint32_t pci_config_read32(struct pci_device_location location, uint16_t offset);

static inline uint16_t pci_read_vendor_id(struct pci_device_location location) {
    return pci_config_read16(location, PCI_CONFIG_VENDOR_ID);
}

static inline uint16_t pci_read_device_id(struct pci_device_location location) {
    return pci_config_read16(location, PCI_CONFIG_DEVICE_ID);
}

static inline struct pci_device_info pci_read_device_info(struct pci_device_location location) {
    uint32_t fields = pci_config_read32(location, PCI_CONFIG_REVISION_ID);
    return (struct pci_device_info) {
        .class_code = (fields & 0xFF000000) >> 24,
        .subclass_code = (fields & 0x00FF0000) >> 16,
        .programming_interface = (fields & 0x0000FF00) >> 8,
        .revision_id = (fields & 0x000000FF) >> 0,
    };
}

static inline uint8_t pci_read_header_type(struct pci_device_location location) {
    return pci_config_read8(location, PCI_CONFIG_HEADER_TYPE);
}

#endif /* _KERNEL_HAL_PCI_H */
