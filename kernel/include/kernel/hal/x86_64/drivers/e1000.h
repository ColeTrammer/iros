#ifndef _KERNEL_HAL_X86_64_DRIVERS_E1000_H
#define _KERNEL_HAL_X86_64_DRIVERS_E1000_H 1

#include <kernel/hal/x86_64/drivers/pci.h>

void init_intel_e1000(struct pci_configuration *config);

#endif /* _KERNEL_HAL_X86_64_DRIVERS_E1000_H */