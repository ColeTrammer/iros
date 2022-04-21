#include <sys/types.h>
#include <kernel/hal/devices.h>
#include <kernel/hal/hal.h>
#include <kernel/hal/hw_device.h>
#include <kernel/hal/irqs.h>
#include <kernel/hal/output.h>
#include <kernel/hal/pci.h>
#include <kernel/hal/processor.h>
#include <kernel/util/init.h>

#include <kernel/arch/x86/asm_utils.h>
#include <kernel/hal/isa_driver.h>
#include <kernel/hal/x86/acpi.h>
#include <kernel/hal/x86/drivers/local_apic.h>
#include <kernel/hal/x86/drivers/pic.h>
#include <kernel/hal/x86/drivers/serial.h>

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
}

static bool s_found_acpi_tables;

void init_cpus(void) {
#ifndef KERNEL_DISABLE_ACPI
    // Parse the acpi tables now that dynamic memory allocation is available.
    s_found_acpi_tables = init_acpi();
#else
    s_found_acpi_tables = false;
#endif

    if (s_found_acpi_tables) {
        init_local_apic_irq_handlers();
    }

#ifdef KERNEL_USE_PIC
    init_pic();
#else
    if (s_found_acpi_tables) {
        disable_pic();
    } else {
        init_pic();
    }
#endif

    if (!s_found_acpi_tables) {
        // Without acpi, just create the bsp and just assume only 1 core.
        struct processor *bsp = create_processor(0);
        add_processor(bsp);
    }

    init_bsp(get_bsp());
}

bool found_acpi_tables() {
    return s_found_acpi_tables;
}

void init_smp(void) {
    if (!found_acpi_tables()) {
        return;
    }

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

void enumerate_devices(void) {
    enumerate_pci_devices();
    enumerate_isa_devices();
    init_virtual_devices();
}

static struct hw_device s_root_device = {
    .name = "x86 Motherboard",
    .children = INIT_LIST(s_root_device.children),
    .tree_lock = SPINLOCK_INITIALIZER,
    .ref_count = 1,
    .status = HW_STATUS_ACTIVE,
    .id = { .type = HW_TYPE_NONE },
};

struct hw_device *root_hw_device(void) {
    return &s_root_device;
}
