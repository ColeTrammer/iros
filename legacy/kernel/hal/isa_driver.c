#include <kernel/hal/hw_device.h>
#include <kernel/hal/isa_driver.h>
#include <kernel/hal/output.h>

#define ISA_DRIVER_DEBUG

static struct list_node isa_drivers = INIT_LIST(isa_drivers);

void register_isa_driver(struct isa_driver *driver) {
#ifdef ISA_DRIVER_DEBUG
    debug_log("Registering ISA driver: [ %s ]\n", driver->name);
#endif /* ISA_DRIVER_DEBUG */
    list_append(&isa_drivers, &driver->list);
}

void enumerate_isa_devices(void) {
    list_for_each_entry(&isa_drivers, driver, struct isa_driver, list) {
        struct hw_device *parent = root_hw_device();
        driver->detect_devices(parent);
    }
}
