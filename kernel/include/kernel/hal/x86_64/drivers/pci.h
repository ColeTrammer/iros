#ifndef _KERNEL_HAL_X86_64_DRIVERS_PCI_H
#define _KERNEL_HAL_X86_64_DRIVERS_PCI_H 1

#include <stdint.h>

#include <kernel/arch/x86_64/asm_utils.h>

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

#define PCI_BUS_MAX  256
#define PCI_SLOT_MAX 32
#define PCI_FUNC_MAX 8

#define PCI_MULTI_FUNCTION_FLAG 0x80

#define PCI_CLASS_MASS_STORAGE      0x1
#define PCI_SUBCLASS_IDE_CONTROLLER 0x1

#define PCI_VENDOR_INTEL       0x8086
#define PCI_DEVICE_INTEL_E1000 0x100E

#define PCI_VENDOR_BOCHS     0x1234
#define PCI_DEVICE_BOCHS_VGA 0x1111

struct pci_configuration {
    uint16_t vendor_id;
    uint16_t device_id;

    uint16_t command;
    uint16_t status;

    uint8_t revision_id;
    uint8_t prog_if;
    uint8_t subclass;
    uint8_t class_code;

    uint8_t cache_line_size;
    uint8_t latency_timer;
    uint8_t header_type;
    uint8_t bist;

    uint32_t bar[6];

    uint32_t cis_pointer;

    uint16_t subsystem_vender_id;
    uint16_t subsystem_id;

    uint32_t expansion_rom_address;

    uint8_t capabilites_pointer;
    uint8_t reserved0[3];

    uint8_t reserved1[4];

    uint8_t interrupt_line;
    uint8_t interrupt_pin;
    uint8_t min_grant;
    uint8_t max_latency;

    uint8_t bus;
    uint8_t slot;
    uint8_t func;
} __attribute__((packed)) __attribute__((aligned(2)));

static inline uint32_t pci_make_config_address(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    return (uint32_t)(0x80000000U | (((uint32_t) bus) << 16U) | (((uint32_t) slot) << 11U) | (((uint32_t) func) << 8U) |
                      (((uint32_t) offset) & 0xFC));
}

static inline uint16_t pci_read_config_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    // Tell controller which address to read from
    outl(PCI_CONFIG_ADDRESS, pci_make_config_address(bus, slot, func, offset));

    // Read that value
    uint32_t value = inl(PCI_CONFIG_DATA);

    // If the offset is not aligned, shift it over 16 bits.
    return (uint16_t)((value >> ((offset & 2) * 8)) & 0xFFFF);
}

static inline void pci_write_config_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t value) {
    // Tell controller which address to read/write from
    outl(PCI_CONFIG_ADDRESS, pci_make_config_address(bus, slot, func, offset));

    // Read that value
    uint32_t to_write = inl(PCI_CONFIG_DATA);

    // Set the correct half of the word to be value
    if (offset & 2) {
        to_write = (to_write & 0x0000FFFF) | (value >> 16);
    } else {
        to_write = (to_write & 0xFFFF0000) | value;
    }

    // Output new value
    outl(PCI_CONFIG_DATA, to_write);
}

static inline uint16_t pci_get_vendor(uint8_t bus, uint8_t slot, uint8_t function) {
    return pci_read_config_word(bus, slot, function, 0);
}

static inline void pci_read_configuation(uint8_t bus, uint8_t slot, uint8_t func, struct pci_configuration *config) {
    uint16_t *buffer = (uint16_t *) config;
    for (uint16_t offset = 0; offset < sizeof(struct pci_configuration); offset += sizeof(uint16_t)) {
        buffer[offset / sizeof(uint16_t)] = pci_read_config_word(bus, slot, func, offset);
    }

    config->bus = bus;
    config->slot = slot;
    config->func = func;
}

static inline void pci_enable_bus_mastering(struct pci_configuration *config) {
    config->command |= (1 << 2) | (1 << 0); // Enable the pci device to respond to pic and io accesses

    pci_write_config_word(config->bus, config->slot, config->func, ((uintptr_t) &config->command) - ((uintptr_t) config), config->command);
}

bool pci_config_for_class(uint8_t class_code, uint8_t subclass, struct pci_configuration *config);
void init_pci();

#endif /* _KERNEL_HAL_X86_64_DRIVERS_PCI_H */