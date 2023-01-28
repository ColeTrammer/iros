#ifndef _KERNEL_HAL_ISA_DRIVER_H
#define _KERNEL_HAL_ISA_DRIVER_H 1

#include <kernel/util/list.h>

struct hw_device;

struct isa_driver {
    const char *name;
    struct list_node list;
    void (*detect_devices)(struct hw_device *parent);
};

void register_isa_driver(struct isa_driver *driver);
void enumerate_isa_devices(void);

#endif /* _KERNEL_HAL_ISA_DRIVER_H */
