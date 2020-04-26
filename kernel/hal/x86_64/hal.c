#include <sys/types.h>
#include <kernel/hal/hal.h>
#include <kernel/hal/irqs.h>
#include <kernel/hal/output.h>

#include <kernel/hal/devices.h>
#include <kernel/hal/ptmx.h>

#include <kernel/hal/x86_64/drivers/ata.h>
#include <kernel/hal/x86_64/drivers/cmos.h>
#include <kernel/hal/x86_64/drivers/fdc.h>
#include <kernel/hal/x86_64/drivers/keyboard.h>
#include <kernel/hal/x86_64/drivers/mouse.h>
#include <kernel/hal/x86_64/drivers/pci.h>
#include <kernel/hal/x86_64/drivers/pic.h>
#include <kernel/hal/x86_64/drivers/pit.h>
#include <kernel/hal/x86_64/drivers/serial.h>
#include <kernel/hal/x86_64/gdt.h>

void init_hal() {
    init_irqs();

    init_pic();
    init_serial_ports();

    init_gdt();

    // Read cmos now so that kernel time is initialized ASAP
    init_cmos();

    debug_log("Finished Initializing HAL\n");
}

void init_drivers() {
    init_keyboard();
    init_mouse();
    init_fdc();
    init_ata();
    init_pit();
    init_serial_port_device(SERIAL_COM1_PORT);
    init_virtual_devices();
    init_pci();

    if (!kernel_use_graphics()) {
        init_vga_device();
    }

    init_ptmx();

    debug_log("Finished Initializing Drivers\n");
}

static bool use_graphics;

void kernel_enable_graphics(void) {
    use_graphics = true;
    debug_log("kernel graphics enabled\n");
}

bool kernel_use_graphics(void) {
    return use_graphics;
}