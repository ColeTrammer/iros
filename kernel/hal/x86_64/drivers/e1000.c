#include <stdbool.h>
#include <stdlib.h>

#include <kernel/hal/output.h>
#include <kernel/hal/x86_64/drivers/e1000.h>

static bool has_eeprom(struct e1000_data *data) {
    (void) data;
    return true;
}

void init_intel_e1000(struct pci_configuration *config) {
    debug_log("Found intel e1000 netword card: [ %u ]\n", config->interrupt_line);
    pci_enable_bus_mastering(config);

    assert(!(config->bar[0] & 1)); // Mem base
    assert(config->bar[1] & 1);    // Port base

    struct e1000_data *data = malloc(sizeof(struct e1000_data));
    data->mem_io_phys_base = config->bar[0];
    data->io_port_base = config->bar[1] & ~1;



    debug_log("IO Bases: [ %#lX, %#X ]\n", data->mem_io_phys_base, data->io_port_base);

    debug_log("Has EEPROM: [ %s ]\n", has_eeprom(data) ? "true" : "false");

    free(data);
}