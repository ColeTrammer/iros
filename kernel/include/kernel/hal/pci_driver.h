#ifndef _KERNEL_HAL_PCI_DRIVER_H
#define _KERNEL_HAL_PCI_DRIVER_H 1

#include <kernel/hal/hw_device.h>
#include <kernel/util/init.h>

struct pci_driver;

struct pci_driver_ops {};

struct pci_driver {
    const char *name;
    struct pci_driver_id *pci_id_table;
    size_t pci_id_table_size;
    struct pci_driver_ops *ops;
};

void register_pci_driver(struct pci_driver *driver);

#define PCI_DRIVER_INIT(name)                                     \
    static void init_##name(void) { register_pci_driver(&name); } \
    INIT_FUNCTION(init_##name, driver)

#endif /* _KERNEL_HAL_PCI_DRIVER_H */
