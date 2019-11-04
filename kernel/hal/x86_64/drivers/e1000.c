#include <stdbool.h>
#include <stdlib.h>

#include <kernel/hal/output.h>
#include <kernel/hal/x86_64/drivers/e1000.h>
#include <kernel/mem/vm_allocator.h>

#define E1000_EEPROM_REG 0x0014

static uint32_t read_command(struct e1000_data *data, uint16_t offset) {
    return *((volatile uint32_t*) (data->mem_io_phys_base + offset));
}

static void write_command(struct e1000_data *data, uint16_t offset, uint32_t value) {
    *((volatile uint32_t*) (data->mem_io_phys_base + offset)) = value;
}

static bool has_eeprom(struct e1000_data *data) {
    // For some reason we need to check 1000 times
    write_command(data, E1000_EEPROM_REG, 1);
    for (int i = 0; i < 1000; i++) {
        if (read_command(data, E1000_EEPROM_REG) & 0x10) {
            return true;
        }
    }

    return false;
}

static uint32_t read_eeprom(struct e1000_data *data, uint8_t addr) {
    write_command(data, E1000_EEPROM_REG, 1U | (((uint32_t) addr) << 8U));

    uint32_t val;
    while (!((val = read_command(data, E1000_EEPROM_REG)) & (1U << 4U)));

    return (uint16_t) ((val >> 16) & 0xFFFF);
}

void init_intel_e1000(struct pci_configuration *config) {
    debug_log("Found intel e1000 netword card: [ %u ]\n", config->interrupt_line);
    pci_enable_bus_mastering(config);

    assert(!(config->bar[0] & 1)); // Mem base
    assert(config->bar[1] & 1);    // Port base

    struct e1000_data *data = malloc(sizeof(struct e1000_data));
    data->mem_io_phys_base = (uintptr_t) create_phys_addr_mapping(config->bar[0]);
    data->io_port_base = config->bar[1] & ~1;

    debug_log("IO Bases: [ %#X, %#X ]\n", config->bar[0], data->io_port_base);

    assert(has_eeprom(data));
    debug_log("Has EEPROM: [ %s ]\n", "true");

    uint8_t mac[6];
    uint32_t res = read_eeprom(data, 0);
    mac[0] = res & 0xFF;
    mac[1] = res >> 8;
    res = read_eeprom(data, 1);
    mac[2] = res & 0xFF;
    mac[3] = res >> 8;
    res = read_eeprom(data, 2);
    mac[4] = res & 0xFF;
    mac[5] = res >> 8;

    debug_log("MAC Address: [ %02x:%02x:%02x:%02x:%02x:%02x ]\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    free(data);
}