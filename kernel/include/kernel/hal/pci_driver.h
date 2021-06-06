#ifndef _KERNEL_HAL_PCI_DRIVER_H
#define _KERNEL_HAL_PCI_DRIVER_H 1

#include <kernel/hal/hw_device.h>
#include <kernel/hal/pci.h>
#include <kernel/util/init.h>

struct pci_driver;

struct pci_driver_ops {
    struct pci_device *(*create)(struct hw_device *parent, struct pci_device_location location, struct pci_device_id id,
                                 struct pci_device_info info);
};

struct pci_driver {
    const char *name;
    struct pci_device_id *device_id_table;
    size_t device_id_count;
    struct pci_driver_ops *ops;
    struct list_node list;
    bool is_default;
};

void pci_register_driver(struct pci_driver *driver);
void pci_unregister_driver(struct pci_driver *driver);
struct pci_driver *pci_find_driver(struct pci_device_id id, struct pci_device_info info);

#define PCI_DRIVER_INIT(name)                                     \
    static void init_##name(void) { pci_register_driver(&name); } \
    INIT_FUNCTION(init_##name, driver)

#endif /* _KERNEL_HAL_PCI_DRIVER_H */
