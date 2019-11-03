#include <kernel/hal/output.h>
#include <kernel/hal/x86_64/drivers/e1000.h>

void init_intel_e1000(struct pci_configuration *config) {
    debug_log("Found intel e1000 netword card: [ %u ]\n", config->interrupt_line);
}