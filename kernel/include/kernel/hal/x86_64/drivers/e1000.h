#ifndef _KERNEL_HAL_X86_64_DRIVERS_E1000_H
#define _KERNEL_HAL_X86_64_DRIVERS_E1000_H 1

#include <stdint.h>

#include <kernel/hal/x86_64/drivers/pci.h>

struct e1000_data {
    uintptr_t mem_io_phys_base;
    uint16_t io_port_base;
};

void init_intel_e1000(struct pci_configuration *config);

#endif /* _KERNEL_HAL_X86_64_DRIVERS_E1000_H */