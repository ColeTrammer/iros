#include <sys/types.h>
#include <kernel/hal/devices.h>
#include <kernel/hal/hal.h>
#include <kernel/hal/irqs.h>
#include <kernel/hal/output.h>
#include <kernel/hal/ptmx.h>

#include <kernel/arch/x86_64/asm_utils.h>
#include <kernel/hal/x86_64/drivers/ata.h>
#include <kernel/hal/x86_64/drivers/cmos.h>
#include <kernel/hal/x86_64/drivers/fdc.h>
#include <kernel/hal/x86_64/drivers/keyboard.h>
#include <kernel/hal/x86_64/drivers/mouse.h>
#include <kernel/hal/x86_64/drivers/pci.h>
#include <kernel/hal/x86_64/drivers/pic.h>
#include <kernel/hal/x86_64/drivers/pit.h>
#include <kernel/hal/x86_64/drivers/serial.h>
#include <kernel/hal/x86_64/drivers/vmware_back_door.h>
#include <kernel/hal/x86_64/gdt.h>

static bool supports_rdrand;

bool cpu_supports_rdrand(void) {
    return supports_rdrand;
}

static void detect_cpu_features(void) {
    uint32_t a, b, c, d;
    cpuid(CPUID_FEATURES, &a, &b, &c, &d);
    supports_rdrand = !!(c & CPUID_ECX_RDRAND);
}

void init_hal() {
    detect_cpu_features();
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
    init_vmware_back_door();
    init_fdc();
    init_ata();
    init_pit();
    init_serial_port_device(SERIAL_COM1_PORT, 0);
    init_virtual_devices();
    init_pci();

    if (!kernel_use_graphics()) {
        init_vga_device();
    }

    init_ptmx();

    debug_log("Finished Initializing Drivers\n");
}

static bool use_graphics = true;

void kernel_disable_graphics(void) {
    use_graphics = false;
    debug_log("kernel graphics disabled\n");
}

bool kernel_use_graphics(void) {
    return use_graphics;
}
