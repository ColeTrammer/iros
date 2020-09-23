#include <sys/types.h>
#include <kernel/hal/devices.h>
#include <kernel/hal/hal.h>
#include <kernel/hal/irqs.h>
#include <kernel/hal/output.h>
#include <kernel/hal/processor.h>

#include <kernel/arch/x86_64/asm_utils.h>
#include <kernel/hal/x86_64/acpi.h>
#include <kernel/hal/x86_64/drivers/ata.h>
#include <kernel/hal/x86_64/drivers/cmos.h>
#include <kernel/hal/x86_64/drivers/fdc.h>
#include <kernel/hal/x86_64/drivers/keyboard.h>
#include <kernel/hal/x86_64/drivers/local_apic.h>
#include <kernel/hal/x86_64/drivers/mouse.h>
#include <kernel/hal/x86_64/drivers/pci.h>
#include <kernel/hal/x86_64/drivers/pic.h>
#include <kernel/hal/x86_64/drivers/pit.h>
#include <kernel/hal/x86_64/drivers/ps2.h>
#include <kernel/hal/x86_64/drivers/serial.h>
#include <kernel/hal/x86_64/drivers/vga.h>
#include <kernel/hal/x86_64/drivers/vmware_back_door.h>
#include <kernel/hal/x86_64/gdt.h>

static bool supports_rdrand;
static bool supports_1gb_pages;

bool cpu_supports_rdrand(void) {
    return supports_rdrand;
}

bool cpu_supports_1gb_pages(void) {
    return supports_1gb_pages;
}

static void detect_cpu_features(void) {
    uint32_t a, b, c, d;
    cpuid(CPUID_FEATURES, &a, &b, &c, &d);
    supports_rdrand = !!(c & CPUID_ECX_RDRAND);

    cpuid(CPUID_EXTENDED_FEATURES, &a, &b, &c, &d);
    supports_1gb_pages = !!(d & CPUID_EDX_1GB_PAGES);
}

void init_hal(void) {
    detect_cpu_features();

    // Set the IDT so that CPU execeptions can be handled
    init_irqs();

    // Set up the serial port debug output
    init_serial_ports();

    // Read cmos now so that kernel time is initialized ASAP
    init_cmos();

    debug_log("Finished Initializing HAL\n");
}

void init_cpus(void) {
#ifdef KERNEL_USE_PIC
    init_pic();
#else
    disable_pic();
    init_local_apic_irq_handlers();
#endif /* KERNEL_USE_PIC */

    // Parse the acpi tables now that dynamic memory allocation is available.
    init_acpi();
}

void init_drivers(void) {
    init_keyboard();
    init_mouse();
    init_ps2_controller();
    init_vmware_back_door();
    init_fdc();
    init_ata();
    init_serial_port_device(SERIAL_COM1_PORT, 0);
    init_pit();

    if (!kernel_use_graphics()) {
        init_vga_device();
    }

    init_pci();

    init_virtual_devices();
    debug_log("Finished Initializing Drivers\n");
}

void init_smp(void) {
    init_processor_ipi_messages();
    set_smp_enabled();
    local_apic_start_aps();
}

static bool use_graphics = true;

void kernel_disable_graphics(void) {
    use_graphics = false;
    debug_log("kernel graphics disabled\n");
}

bool kernel_use_graphics(void) {
    return use_graphics;
}
